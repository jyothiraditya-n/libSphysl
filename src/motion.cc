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

	libSphysl::utility::slice_t<double> m; // Masses.
	libSphysl::utility::slice_t<double> x, y, z; // Positions.
	libSphysl::utility::slice_t<double> v_x, v_y, v_z; // Velocities.
	libSphysl::utility::slice_t<double> a_x, a_y, a_z; // Accelerations.
	libSphysl::utility::slice_t<double> F_x, F_y, F_z; // Forces.

	/* The following are only used if we're storing additional terms of the
	 * Maclaurin series for the variables. */
	const size_t length, depth; // Number of entities, number of terms.

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

/* This is the calculator that we will be using for the engines. */
template<bool relativistic, bool smoothed> static void calculator(void *arg);

/* These are helper functions that will be used by the calculator to help avoid
 * needless code reduplication. */
template<bool relativistic> static void simple_helper(arg_t& data);

template<bool relativistic, bool initialised>
static void smoothed_helper(size_t i, arg_t& data);
/* Templating the initialisedness allows the compiler to optimise away a bunch
 * of if-statements, which speeds up performance a fair bit. */

/* Function Definitions */

libSphysl::engine_t libSphysl::motion::classical(libSphysl::sandbox_t* s) {

}

libSphysl::engine_t
libSphysl::motion::classical(libSphysl::sandbox_t* s, size_t smoothing){

}

libSphysl::engine_t libSphysl::motion::relativistic(libSphysl::sandbox_t* s) {

}

libSphysl::engine_t
libSphysl::motion::relativistic(libSphysl::sandbox_t* s, size_t smoothing) {

}

template<bool relativistic, bool smoothed> static void calculator(void *arg) {
	/* Get a reference to our cached data by casting the argument. */
	auto& data = reinterpret_cast<arg_t*>(arg);

	/* Loop across the slices and compute for each entity. */
	size_t i = 0; // Index variable that we'll use for our caching vectors.
	for(const auto& m: data.ms) {
		/* Call the appropriate helper function. */
		if constexpr(!smoothed) {
			simple_helper<relativistic>(data);
			/* Pass forward the template parameter we've been
			 * given. */
		}
		
		else {
			/* This is the only runtime check for whether or not
			 * the data is initialised. */
			if(data.initialised) {
				/* Call the templated function for initialised
				 * caches. */
				smoothed_helper<relativistic, true>(i, data);
			}

			else {
				/* Call the templated function for
				 * unitialised caches. */
				smoothed_helper<relativistic, false>(i, data);
			}
		}

		/* Set forces back to 0 for next simulation step. */
		data.F_x = data.F_y = data.F_z = 0.0;

		/* Step the slices forward. */
		data.F_x++; data.F_y++; data.F_z++;
		data.a_x++; data.a_y++; data.a_z++;
		data.v_x++; data.v_y++; data.v_z++;
		data.x++; data.y++; data.z++;

		/* Update index variable. */
		i++;
	}

	/* Reset the slices back to the beginning. */
	data.F_x.goto_begin(); data.F_y.goto_begin(); data.F_z.goto_begin();
	data.a_x.goto_begin(); data.a_y.goto_begin(); data.a_z.goto_begin();
	data.v_x.goto_begin(); data.v_y.goto_begin(); data.v_z.goto_begin();
	data.x.goto_begin(); data.y.goto_begin(); data.z.goto_begin();

	/* Since by now the data has been initialised, mark it as such. */
	data.initialised = true;
}

template<bool relativistic> static void simple_helper(arg_t& data) {
	/* Calculate acceleration classically. */
	if constexpr(!relativistic) {
		/* F = ma => a = F / m */
		data.a_x = data.F_x / m;
		data.a_y = data.F_y / m;
		data.a_z = data.F_z / m;
	}

	/* Calculate acceleration relativistically. */
	else {
		/* TODO */
	}

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
	if constexpr(!initialised) {
		/* The first and second values in the pair mark the difference
		 * between the last velocity value and the current velocity
		 * value (after we've calculated it) for each term in the
		 * Maclaurin series. (See docs/smoothed_motion.md for more.) */

		/* However, since they start off as garbage values, we will
		 * get ridiculous initial deltas if we don't initialise them
		 * to whatever initial velocities have been input into the
		 * system. */
		data.dv_xs[i][0].first = data.v_x;
		data.dv_ys[i][0].first = data.v_y;
		data.dv_zs[i][0].first = data.v_z;
	}

	for(size_t j = 0; j < data.depth; j++) {
		/* The old new is the new old. Migrate the cache to prepare to
		 * compute differences. */
		data.da_xs[i][j].second = data.da_xs[i][j].first;
		data.da_ys[i][j].second = data.da_ys[i][j].first;
		data.da_zs[i][j].second = data.da_zs[i][j].first;

		data.dv_xs[i][j].second = data.dv_xs[i][j].first;
		data.dv_ys[i][j].second = data.dv_ys[i][j].first;
		data.dv_zs[i][j].second = data.dv_zs[i][j].first;
	}

	/* Calculate acceleration classically. */
	if constexpr(!relativistic) {
		/* F = ma => a = F / m */
		data.a_x = data.da_xs[i][0].first = data.F_x / data.m;
		data.a_y = data.da_ys[i][0].first = data.F_y / data.m;
		data.a_z = data.da_zs[i][0].first = data.F_z / data.m;
	}

	/* Calculate acceleration relativistically. */
	else {
		/* TODO */
	}

	if constexpr(!initialised) {
		/* Similar to what we did for the cached velocities, except
		 * since we deferred calculating the acceleration, we're now
		 * copying to the 'old' cache rather than the 'new' cache.
		 * Either way, the effect is the same. */
		data.da_xs[i][0].second = data.a_x;
		data.da_ys[i][0].second = data.a_y;
		data.da_zs[i][0].second = data.a_z;
	}
}