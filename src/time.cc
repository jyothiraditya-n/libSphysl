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

/* Including Standard Libraries */

#include <chrono>

/* Including Library Headerfiles */

#include <libSphysl/time.h>
#include <libSphysl/utility.h>

/* Structure Declarations */

/* This is the argument that's gonig to be passed to the calculator. */
struct arg_t {
	/* The simulation data we are in charge of. */
	double &t, &delta_t;
	size_t &tick;

	/* We need to know how much time has elapsed since the last time the
	 * clock was updated. */
	std::chrono::high_resolution_clock::time_point last;
	bool initialised; // To keep track of whether last has been set.

	/* The constraints on the delta_t. */
	const double &min, &max;
};

/* Function definitions */

/* The engines do fairly similar things, so to avoid source code duplication we
 * write only a single calculator. However, for performance, this is templated
 * so the compiler can get rid of conditionals and duplicate code as needed. */

template<bool constrained, bool constant> static void calculator(void* arg) {
	/* Get a reference to our cached data by casting the argument. */
	auto &data = *reinterpret_cast<arg_t*>(arg);

	/* If the time change is not constant, get the change in clock time. */
	if constexpr(!constant) {
		/* If last hasn't been initialised, initialise it and mark that
		 * we've done that. */
		if(!data.initialised) {
			data.last = std::chrono::high_resolution_clock::now();
			data.initialised = true;
		}

		/* Get the current system time and compare that with the time
		 * the clock was last updated. */
		const auto now = std::chrono::high_resolution_clock::now();
		const auto duration = std::chrono::duration<double>(
			now - data.last
		);

		/* Compute the change in time and update the cached time. */
		data.delta_t = duration.count();
		data.last = now;
	}

	/* If the time change is constrained, make sure it is within limits. */
	if constexpr(constrained) {
		data.delta_t = data.delta_t > data.max? data.max: data.delta_t;
		data.delta_t = data.delta_t < data.min? data.min: data.delta_t;
	}

	/* Update the time and move forward a tick. */
	data.t += data.delta_t;
	data.tick++;
}

/* All of the engine generators need to get the core simulation data, so to
 * avoid code reduplication, it should be its own helper function. */

struct data_t {
	double &t, &delta_t;
	size_t &tick;
};

static data_t get_data(libSphysl::sandbox_t* s) {
	/* Get the variables we need from the config and then reset them to
	 * their default values. This very simply guarantees the std::variant's
	 * will be of the correct type. */
	auto& t = s -> config["time"];
	t = libSphysl::default_time;

	auto& delta_t = s -> config["time change"];
	delta_t = libSphysl::default_time_change;

	auto& tick = s -> config["simulation tick"];
	tick = libSphysl::default_simulation_tick;

	/* Return the references to the correct data type of the variants. */
	return {
		std::get<double>(t),
		std::get<double>(delta_t),
		std::get<size_t>(tick)
	};
}

/* The engine generators' code is a bit repetitive, but there's nothing that
 * can be done about that without getting into needlessly funky C preprocessor
 * code. */

libSphysl::engine_t libSphysl::time::realtime(libSphysl::sandbox_t* s) {
	/* The engine we're generating. */
	libSphysl::engine_t engine;

	/* The compiler will generate the appropriate overloads from the
	 * templates it's been provided. */
	engine.calculator = calculator<false, false>;
	engine.destructor = libSphysl::utility::destructor<arg_t>;

	/* Get the core simulation data. */
	auto [t, delta_t, tick] = get_data(s);

	/* Create a new argument on the heap. */
	auto arg = new arg_t{
		t, delta_t, tick, // Core simulation data.
		{}, false, // Set up the clock.
		{}, {} // We don't have any constraints.
	};

	// Only one calculation in this engine; return the generated engine.
	engine.args.push_back(reinterpret_cast<void*>(arg));
	return engine;
}

libSphysl::engine_t libSphysl::time::constrained(libSphysl::sandbox_t* s) {
	/* The engine we're generating. */
	libSphysl::engine_t engine;

	/* The compiler will generate the appropriate overloads. */
	engine.calculator = calculator<true, false>;
	engine.destructor = libSphysl::utility::destructor<arg_t>;

	/* Get the core simulation data. */
	auto [t, delta_t, tick] = get_data(s);

	/* Get our constraints. */ 
	auto& min = s -> config["minimum time change"];
	min = libSphysl::default_minimum_time_change;

	auto& max = s -> config["maximum time change"];
	max = libSphysl::default_maximum_time_change;

	auto arg = new arg_t{
		t, delta_t, tick, // Core simulation data.
		{}, false, // Set up the clock.

		std::get<double>(min), std::get<double>(max)
		// Set up the constraints.
	};

	/* Finish generating the engine. */
	engine.args.push_back(reinterpret_cast<void*>(arg));
	return engine;
}

libSphysl::engine_t libSphysl::time::constant(libSphysl::sandbox_t* s) {
	/* The engine we're generating. */
	libSphysl::engine_t engine;

	/* The compiler will generate the appropriate overloads. */
	engine.calculator = calculator<false, true>;
	engine.destructor = libSphysl::utility::destructor<arg_t>;

	/* Get the core simulation data. */
	auto [t, delta_t, tick] = get_data(s);

	/* Create a new argument on the heap. */
	auto arg = new arg_t{
		t, delta_t, tick, // Core simulation data.
		{}, false, // Set up the clock.
		{}, {} // We don't have any constraints.
	};

	// Only one calculation in this engine; return the generated engine.
	engine.args.push_back(reinterpret_cast<void*>(arg));
	return engine;
}