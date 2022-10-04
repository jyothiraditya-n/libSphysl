/* The Sphysl Project Copyright (C) 2022 Jyothiraditya Nellakra
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>. */

/* Including Library Headerfiles */

#include <libSphysl/motion.h>
#include <libSphysl/utility.h>

/* Structure Declarations */

/* This is the argument that's gonig to be passed to the calculators. */
struct arg_t {
	const double& delta_t; // Time elapsed per simulation tick.
	const double& c; // The speed of light.

	libSphysl::utility::slice_t<double> m; // Masses.
	libSphysl::utility::slice_t<double> x, y, z; // Positions.
	libSphysl::utility::slice_t<double> v_x, v_y, v_z; // Velocities.
	libSphysl::utility::slice_t<double> a_x, a_y, a_z; // Accelerations.
	libSphysl::utility::slice_t<double> F_x, F_y, F_z; // Forces.

	/* The following are only used if we're storing additional terms of the
	 * Maclaurin series for the variables. */

	const size_t depth; // Number of terms.

	bool initialised;
	/* This stores whether or not we've put the current values into the
	 * 'old' columns of our cache upon starting the simulation. */

	/* These are our data caches for calculating derivatives on the fly in
	 * order to compute Maclaurin series. */
	std::vector<std::vector<std::pair<double, double>>>
		dv_xs, dv_ys, dv_zs, da_xs, da_ys, da_zs;

	std::vector<double> coeffs; // Precomptued to avoid costly factorials.
};

/* Function Declarations */

/* This is the templated function that generates all the engines. */
template<bool relativistic, bool smoothed>
static libSphysl::engine_t generator(
	libSphysl::sandbox_t* s, size_t smoothing
);

/* These are functions that helps avoid repeating code when we're fetching
 * doubles from the config nad vectors of doubles from the database. */

static double& get_double(libSphysl::sandbox_t* s, const std::string& id);

static std::vector<double>& get_doubles(
	libSphysl::sandbox_t* s, const std::string& id
);

/* This function generates a slice_t<double> of the type we need. */
static libSphysl::utility::slice_t<double> get_slice(
	std::vector<double> &v, size_t start, size_t stop
);

/* This function returns the factorial of a positive integer. */
static size_t factorial(const size_t n);

/* This is the calculator that we will be using for the engines. */
template<bool relativistic, bool smoothed> static void calculator(void *arg);

/* These are helper functions that will be used by the calculator to help avoid
 * needless code reduplication. */
template<bool relativistic> static void simple_helper(size_t i, arg_t& data);

template<bool relativistic, bool initialised>
static void smoothed_helper(size_t i, arg_t& data);
/* Templating the initialisedness allows the compiler to optimise away a bunch
 * of if-statements, which speeds up performance a fair bit. */

/* This is used to avoid repeating code for calculating the acceleration. (Or
 * what is equivalent to the current 0th derivative of the acceleration value
 * in the case of the smoothed function.) */
template<bool relativistic> static void calculate_acceleration(arg_t& data);

/* This is used to avoid repeating code when integrating acceleration for
 * velocity and velocity for distance. */
static void smoothly_integrate(
	double& I_x, double& I_y, double& I_z,
	double& f_x, double& f_y, double& f_z,

	std::vector<std::pair<double, double>>& d_xs,
	std::vector<std::pair<double, double>>& d_ys,
	std::vector<std::pair<double, double>>& d_zs,

	const arg_t& data
);

/* Function Definitions */

libSphysl::engine_t libSphysl::motion::classical(libSphysl::sandbox_t* s) {
	/* Call the templated generator with the appropriate parameters. */
	return generator<false, false>(s, 0);
}

libSphysl::engine_t
libSphysl::motion::classical(libSphysl::sandbox_t* s, size_t smoothing){
	/* Call the templated generator with the appropriate parameters. */
	return generator<false, true>(s, smoothing);
}

libSphysl::engine_t libSphysl::motion::relativistic(libSphysl::sandbox_t* s) {
	/* Call the templated generator with the appropriate parameters. */
	return generator<true, false>(s, 0);
}

libSphysl::engine_t
libSphysl::motion::relativistic(libSphysl::sandbox_t* s, size_t smoothing) {
	/* Call the templated generator with the appropriate parameters. */
	return generator<true, true>(s, smoothing);
}

template<bool relativistic, bool smoothed>
static libSphysl::engine_t generator(
	libSphysl::sandbox_t* s, size_t smoothing
){
	/* This is the engine we will be returning. */
	libSphysl::engine_t engine;

	/* Set up the engine with the correct parameters. */
	engine.calculator = calculator<relativistic, smoothed>;
	engine.destructor = libSphysl::utility::destructor<arg_t>;

	/* Get the variables we need from the config. */
	const auto& entities = std::get<size_t>(
		s -> config_get("entity count")
	);

	const auto threads = s -> threads.size(); // Not stored in config.

	const auto& delta_t = get_double(s, "time change");
	const auto& c = get_double(s, "light speed");

	/* Get the variables we need from the database. */
	auto& ms = get_doubles(s, "mass");

	auto& xs = get_doubles(s, "x position");
	auto& ys = get_doubles(s, "y position");
	auto& zs = get_doubles(s, "z position");

	auto& v_xs = get_doubles(s, "x velocity");
	auto& v_ys = get_doubles(s, "y velocity");
	auto& v_zs = get_doubles(s, "z velocity");

	auto& a_xs = get_doubles(s, "x acceleration");
	auto& a_ys = get_doubles(s, "y acceleration");
	auto& a_zs = get_doubles(s, "z acceleration");

	auto& F_xs = get_doubles(s, "x force");
	auto& F_ys = get_doubles(s, "y force");
	auto& F_zs = get_doubles(s, "z force");

	/* Figure out the division of labour; which threads are going to be
	 * responsible for which range of the entities in the system. */
	const auto ranges = libSphysl::utility::divide_range(
		0, entities, threads
	);

	/* This is a lambda function that's called immediately. The main reason
	 * it exists is to get us the appropriate initialiser funciton for
	 * creating an arg_t object with respect to whether we're applying
	 * smoothing or not. */

	auto initialiser = [&]() {
		/* We return one of two lambda functions, both of which have
		 * the same parameters and return type, which is why this is
		 * legal as far as the compiler is concerned. */

		if constexpr(!smoothed) return [&](
			size_t start, size_t stop, size_t depth
		){
			(void) depth;
			/* We don't use the depth parameter unless we're going
			 * to apply smoothing. */

			/* Generate it on the heap, it will be cleaned up when
			 * libSphysl::utility::destructor<arg_t>() runs. */
			return new arg_t{
				delta_t, c,
				
				/* Call our helper functions as appropriate. */
				get_slice(ms, start, stop),

				get_slice(xs, start, stop),
				get_slice(ys, start, stop),
				get_slice(zs, start, stop),

				get_slice(v_xs, start, stop),
				get_slice(v_ys, start, stop),
				get_slice(v_zs, start, stop),

				get_slice(a_xs, start, stop),
				get_slice(a_ys, start, stop),
				get_slice(a_zs, start, stop),

				get_slice(F_xs, start, stop),
				get_slice(F_ys, start, stop),
				get_slice(F_zs, start, stop),
				
				/* The following values aren't used, so we will
				 * null them out here. */

				{}, {}, // depth, initialised.
				{}, {}, {}, // dv_xs, dv_ys, dv_zs.
				{}, {}, {}, // da_xs, da_ys, da_zs.
				{} // coeffs.
			};
		};

		else return [&](size_t start, size_t stop, size_t depth) {
			/* Each of the d<...>_<...>s in the main structure are
			 * vectors of vectors of pairs with dimensions length
			 * x depth. We create a blank set up of the same here
			 * so that we can copy it into structure a bunch of
			 * times. */
			std::vector<std::vector<std::pair<double, double>>>
				pairs(stop - start, {depth, {0.0, 0.0}});

			/* The coefficients are just a simple set of factorials
			 * up to the depth. Expensive to recompute constantly
			 * at runtime, but not too expensive to wastefully
			 * recompute every time this lambda is run like we're
			 * doing here. */
			std::vector<double> coeffs(depth);

			for(size_t i = 0; i < depth; i++) {
				coeffs[i] = factorial(i + 1);
			}

			return new arg_t{
				delta_t, c,
				
				/* Call our helper functions as appropriate. */
				get_slice(ms, start, stop),

				get_slice(xs, start, stop),
				get_slice(ys, start, stop),
				get_slice(zs, start, stop),

				get_slice(v_xs, start, stop),
				get_slice(v_ys, start, stop),
				get_slice(v_zs, start, stop),

				get_slice(a_xs, start, stop),
				get_slice(a_ys, start, stop),
				get_slice(a_zs, start, stop),

				get_slice(F_xs, start, stop),
				get_slice(F_ys, start, stop),
				get_slice(F_zs, start, stop),

				depth, false, // initialised.
				pairs, pairs, pairs, // dv_xs, dv_ys, dv_zs.
				pairs, pairs, pairs, // da_xs, da_ys, da_zs.
				coeffs
			};
		};
	}();

	/* For all of the ranges, construct an approriate arg_t and add it to
	 * the list of args we have for our engine. */
	for(const auto& i: ranges) {
		const auto arg = initialiser(i.first, i.second, smoothing);
		engine.args.push_back(reinterpret_cast<void*>(arg));
	}

	/* Return the engine we have generated. */
	return engine;
}

static double& get_double(libSphysl::sandbox_t* s, const std::string& id) {
	/* No need to keep typing this sentence again and again! */
	return std::get<double>(s -> config_get(id));
}

static std::vector<double>& get_doubles(
	libSphysl::sandbox_t* s, const std::string& id
){
	/* No need to keep typing this sentence again and again! */
	return std::get<std::vector<double>>(s -> database_get(id));
}

static libSphysl::utility::slice_t<double> get_slice(
	std::vector<double> &v, size_t start, size_t stop
){
	/* No need to keep typing this sentence again and again! */
	return libSphysl::utility::slice_t<double>(v, start, stop);
}

static size_t factorial(const size_t n) {
	/* A simple implementation of a factorial calculation, no further
	 * comments needed. */

	size_t x = 1;

	for(size_t i = 2; i <= n; i++) {
		x *= i;
	}

	return x;
}

template<bool relativistic, bool smoothed> static void calculator(void *arg) {
	/* Get a reference to our cached data by casting the argument. */
	auto& data = *reinterpret_cast<arg_t*>(arg);

	/* Get our helper function. We do this so that we only check if the
	 * data is initialised once per function call at runtime. */
	const auto helper = (!smoothed)? simple_helper<relativistic>:
		(data.initialised? smoothed_helper<relativistic, true>:
		smoothed_helper<relativistic, false>);

	/* Loop across the slices and compute for each entity. */
	size_t i = 0; // Index variable that we'll use for our caching vectors.
	for(const auto& m: data.m) {
		(void) m; // We don't actually use this index variable.

		/* Call the appropriate helper function. */
		helper(i, data);

		/* Set forces back to 0 for next simulation step. */
		data.F_x = data.F_y = data.F_z = 0.0;

		/* Step the slices forward. */
		data.m++;

		data.F_x++; data.F_y++; data.F_z++;
		data.a_x++; data.a_y++; data.a_z++;
		data.v_x++; data.v_y++; data.v_z++;
		data.x++; data.y++; data.z++;

		/* Update index variable. */
		i++;
	}

	/* Reset the slices back to the beginning. */
	data.m.goto_begin();

	data.F_x.goto_begin(); data.F_y.goto_begin(); data.F_z.goto_begin();
	data.a_x.goto_begin(); data.a_y.goto_begin(); data.a_z.goto_begin();
	data.v_x.goto_begin(); data.v_y.goto_begin(); data.v_z.goto_begin();
	data.x.goto_begin(); data.y.goto_begin(); data.z.goto_begin();

	/* Since by now the data has been initialised, mark it as such. */
	data.initialised = true;
}

template<bool relativistic> static void simple_helper(size_t i, arg_t& data) {
	/* Null out the index since we don't use it. */
	(void) i;

	/* Calculate the acceleration value at this moment. The rest is simple
	 * Euler integration; fast but not accurate. */
	calculate_acceleration<relativistic>(data);

	/* v = v_initial + a * delta_t */
	data.v_x += data.a_x * data.delta_t;
	data.v_y += data.a_y * data.delta_t;
	data.v_z += data.a_z * data.delta_t;

	/* x = x_initial + v * delta_t */
	data.x += data.v_x * data.delta_t;
	data.y += data.v_y * data.delta_t;
	data.z += data.v_z * data.delta_t;
}

template<bool relativistic, bool initialised>
static void smoothed_helper(size_t i, arg_t& data) {
	/* Calculate the acceleration value at this moment. */
	calculate_acceleration<relativistic>(data);

	if constexpr(!initialised) {
		/* The first and second values in the pair mark the difference
		 * between the last values and the current values (after we've
		 * calculated them) for each term in the Maclaurin series. (See
		 * docs/smoothed_motion.md for more.) */

		/* However, since they start off as garbage values, we will
		 * get ridiculous initial deltas if we don't initialise them.
		 * The accelerations need to be computed from the forces and
		 * then copied over, and the velocities start off as the values
		 * already entered in the database. */

		/* Copy the acceleration values over. */
		data.da_xs[i][0].first = data.a_x();
		data.da_ys[i][0].first = data.a_y();
		data.da_zs[i][0].first = data.a_z();

		/* Copy the velocity values over. */
		data.dv_xs[i][0].first = data.v_x();
		data.dv_ys[i][0].first = data.v_y();
		data.dv_zs[i][0].first = data.v_z();
	}

	/* The old new is the new old. Migrate the cache to prepare to
	 * compute differences. This will result is second = first when this
	 * function is run with initialised = false, and an actual difference
	 * every other time. */
	for(size_t j = 0; j < data.depth; j++) {
		data.da_xs[i][j].second = data.da_xs[i][j].first;
		data.da_ys[i][j].second = data.da_ys[i][j].first;
		data.da_zs[i][j].second = data.da_zs[i][j].first;

		data.dv_xs[i][j].second = data.dv_xs[i][j].first;
		data.dv_ys[i][j].second = data.dv_ys[i][j].first;
		data.dv_zs[i][j].second = data.dv_zs[i][j].first;
	}

	/* Call our integration helper function twice: once for constructing
	 * velocity using acceleration and once for constructing position
	 * using velocity. */

	smoothly_integrate(
		data.v_x(), data.v_y(), data.v_z(),
		data.a_x(), data.a_y(), data.a_z(),
		data.da_xs[i], data.da_ys[i], data.da_zs[i],
		data
	);

	smoothly_integrate(
		data.x(), data.y(), data.z(),
		data.v_x(), data.v_y(), data.v_z(),
		data.dv_xs[i], data.dv_ys[i], data.dv_zs[i],
		data
	);
}

template<bool relativistic>
static void calculate_acceleration(arg_t& data) {
	/* Calculate acceleration classically. */
	if constexpr(!relativistic) {
		/* F = ma => a = F / m */
		data.a_x = data.F_x / data.m;
		data.a_y = data.F_y / data.m;
		data.a_z = data.F_z / data.m;
	}

	/* Calculate acceleration relativistically. */
	else {
		/* Construct relevant vectors out of data. */
		const libSphysl::utility::vector_t
			F(data.F_x, data.F_y, data.F_z),
			v(data.v_x, data.v_y, data.v_z);

		/* Gamma^2 = 1 / (1 - (v/c)^2) = 1 / (1 - v^2 / c^2). */
		const auto gamma_sq = 1.0 / (1.0 - v.length_sq()
			/ (data.c * data.c));

		/* a_along_v = proj_v(F) / (m gamma^3)  | proj_v() projects
		 * a_perp_v = F - proj_v(F) / (m gamma) | onto the v vector.
		 * The derivation for the same can be found in footnote [1]. */

		const auto a_along_v = v.proj(F)
			/ (data.m * std::pow(gamma_sq, 1.5));

		const auto a_perp_v = (F - v.proj(F))
			/ (data.m * std::pow(gamma_sq, 0.5));

		/* Sum the values and move the components into their respective
		 * variables. */

		const auto a = a_along_v + a_perp_v;
		data.a_x = a.x; data.a_y = a.y; data.a_z = a.z;
	}
}

static void smoothly_integrate(
	double& I_x, double& I_y, double& I_z,
	double& f_x, double& f_y, double& f_z,

	std::vector<std::pair<double, double>>& d_xs,
	std::vector<std::pair<double, double>>& d_ys,
	std::vector<std::pair<double, double>>& d_zs,

	const arg_t& data
){
	/* Grab the current values of the function we are integrating as the
	 * current values of the 0th derivative (which is to say, the function
	 * itself). */
	d_xs[0].first = f_x;
	d_ys[0].first = f_y;
	d_zs[0].first = f_z;

	/* The first term in the Maclaurin series is the same as the unsmoothed
	 * integration, as the power of delta_t is 1 and the coefficient and
	 * respective factorial are also 1. */
	I_x += f_x * data.delta_t;
	I_y += f_y * data.delta_t;
	I_z += f_z * data.delta_t;

	/* Cache 1 / delta_t since multiplication is faster than division,
	 * and we want to minimise the number of times we call the division
	 * operator. */
	const auto I_t = 1 / data.delta_t;

	/* Iterate through all the other terms of the Maclaurin series we are
	 * using to smoothe the reconstructed integrated value. */
	for(size_t i = 1; i < data.depth; i++) {
		/* Compute the nth derivative with respect to time as the value
		 * of the (n - 1)th before and after derivative values divided
		 * by the current time change. */
		d_xs[i].first = (d_xs[i - 1].first - d_xs[i - 1].second) * I_t;
		d_ys[i].first = (d_ys[i - 1].first - d_ys[i - 1].second) * I_t;
		d_zs[i].first = (d_zs[i - 1].first - d_zs[i - 1].second) * I_t;

		/* Multiply the derivative by the correct power of delta_t and
		 * the correct factorial coefficient to compute the respective
		 * term in the Maclaurin series before adding it to the value
		 * of the integrated term we are computing. */

		/* Dvisions and calls to std::pow are slow so let's cache this
		 * value before we use it. */
		const auto factor = std::pow(data.delta_t, i + 1.0)
			/ data.coeffs[i];

		I_x += d_xs[i].first * factor;
		I_y += d_ys[i].first * factor;
		I_z += d_zs[i].first * factor;
	}
}

/* [1] https://en.wikipedia.org/wiki/Relativistic_mechanics#Force */