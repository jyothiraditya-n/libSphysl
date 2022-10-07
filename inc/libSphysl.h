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

#include <cmath>
#include <cstdint>
#include <cstddef>

#include <complex>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

/* Avoiding Header Redefinitions */

#ifndef LIBSPHYSL_H
#define LIBSPHYSL_H 1
namespace libSphysl {

/* Version Information for libSphysl. */

/* Version number incremented every time backwards-compatibility breaks,
 * subversion number increased when new features are introduced. Check (version
 * == <what you need>) and (subversion >= <what you need>) for versioning. */

inline const auto version = 1;
inline const auto subversion = 0;
inline const auto version_name = "Dust on the Floor";

/* Type Definitions Relating to Code */

/* The simulations are set up and run using a sandbox_t structure, and you can
 * have as many simulations running in your program as you want. Abstractly,
 * the simulation schedules and runs calculations on your behalf, manging
 * multi-threading and synchronization for you. */

/* The calculations are implemented as function calls to calculator_t's which
 * take a single void* argument. The notion is that these functions will use
 * reinterpret_cast to change the argument into a pointer to a structure which
 * has cached all the values needed for the calculation. */

/* Calculations that can run concurrently without causing data-races (which is
 * something that needs to be verified by the implementor of each calculation)
 * are stored in an engine_t along with a destructor_t. The understanding is
 * that the arguments taken by the calculator_t are usually heap-allocated with
 * new and need to be freed with delete when the engine_t's destructor is
 * called. This is done by calling the user-provided destructor function. */

struct sandbox_t; // Forward declarations.
struct engine_t;

typedef std::function<void(void* arg)> calculator_t;
typedef std::function<void(engine_t* e)> destructor_t;

struct engine_t {
	calculator_t calculator{};
	std::list<void*> args{};

	destructor_t destructor{};
};

/* Internally, the sandbox_t manages code execution by using threads and
 * worksets. Each thread corresponds (ideally) to a single CPU core and will
 * keep running calculations from simulation start to simulation stop. While
 * doing so, a control thread will monitor progress using start and stop
 * mutexes, signal execution stop using a boolean, and change the listing on
 * each thread as worksets change. */

typedef std::pair<calculator_t, std::vector<void*>> listing_t;

struct thread_t {
	listing_t* listing{}; // The listing is swapped continuously.

	std::thread thread{};
	std::mutex start{}, stop{};
	bool finished = false;
};

/* A workset stores a listing for each thread of computations that can run in
 * parallel, and one is generated when a new engine is added. The implication
 * is that the list of arguments from an engine will be divided evenly between
 * the number of threads and stored in vectors for faster data access. */

struct workset_t {
	std::vector<thread_t>& threads; // The threads are shared.
	std::vector<listing_t> listings{};

	workset_t(sandbox_t* s, const engine_t& e);

	/* Helper function to initialise the threads and synchronise them. */
	void run();
};

/* Type Defininitions Relating to Data */

/* Booleans, size_t's (the system-native unsigned integers), intmax_t's (the
 * system-native signed integers), doubles (system-native floating point
 * numbers) and complex numbers should cover most scenarios for data, and
 * there's binary_t's as well in case you need to store anything else. */

struct binary_t; // Forward declaration.

typedef std::variant<
	bool, size_t, std::intmax_t, double, std::complex<double>,
	binary_t

> data_t;

typedef std::pair<data_t, data_t> data_pair_t;

/* The choice of storing a variant of vectors of each type rather than a vector
 * of variants was done due to performance, since running std::get() on a
 * variant to acquire an lvalue of the data we want is slowed down by the type
 * checks at runtime. Furthermore, this also reduces memory usage. */

/* With this setup, you can run an algorithm on an entire vector by iterating
 * through it and only caching a pointer to the vector itself, rather than
 * having to cache pointers to every single value. */

typedef std::variant<
	std::vector<bool>, std::vector<size_t>, std::vector<std::intmax_t>,
	std::vector<double>, std::vector<std::complex<double>>,
	std::vector<binary_t>

> data_vector_t;

/* binary_t allows for arbitrary data to be kept in the database, but to also
 * be saved and loaded off of the disk by libSphysl itself. However, if the
 * data is a memory structure containing pointers, we can't know to save
 * whatever is pointed to, and we certainly can't load it back in the same
 * location. */

/* Hence, only store pointers to relocatable data in the binary_t structure. If
 * you need to cache memory locations for performance, do so elsewhere and
 * reconstruct the cache on simulation restarts. */

struct binary_t {
	void* value{};
	size_t length{};
};

/* The choice of strings and the default map are done both to make the code
 * setting and getting data more readable and to discourage rummaging through
 * the map in hot loops (by making the performance prospect abysmal). */

/* You should cache pointers to relevant vectors and variables as needed before
 * the simulation runs. */

typedef std::map<std::string, data_vector_t> database_t;
typedef std::map<std::string, data_t> config_t;

/* Constants Declarations */

/* The following are stored into a sandbox_t when an engine generator needs to
 * create a config or database entry it depends on. */

inline config_t default_configs{
	{"entity count", size_t{0}}, // units

	{"time", 0.0}, // seconds
	{"simulation tick", size_t{0}}, // units
	{"time change", std::pow(10.0, -6.0)}, // seconds

	{"minimum time change", std::pow(10.0, -7.0)}, // seconds
	{"maximum time change", std::pow(10.0, -5.0)},

	{"gravitational constant", 6.67430 * std::pow(10.0, -11.0)},
	// Newton metre^2 / kilogramme^2

	{"speed of light", 2.99792458 * std::pow(10.0, 8.0)} // metres / second
};

inline config_t default_entry_values{
	{"x position", 0.0}, // metres
	{"y position", 0.0},
	{"z position", 0.0},

	{"x velocity", 0.0}, // metres/second
	{"y velocity", 0.0},
	{"z velocity", 0.0},

	{"x acceleration", 0.0}, // metres/second^2
	{"y acceleration", 0.0},
	{"z acceleration", 0.0},

	{"x force", 0.0}, // Netwons
	{"y force", 0.0},
	{"z force", 0.0},

	{"mass", 1.0} // kilogrammes
};

/* The following defines the ranges for randomly generating data for variables
 * in a sandbox_t's database. */

inline std::map<std::string, data_pair_t> default_entry_ranges{};

/* Main Type Definition */

/* Putting it all together, we have the sandbox_t itself, storing data and code
 * components as discussed earlier */

struct sandbox_t {
	std::vector<workset_t> worksets{};
	std::vector<thread_t> threads{};

	database_t database{};
	config_t config{};

	std::list<engine_t> engines;

	/* The understanding is that the engine(s) will actually be generated
	 * by a generator function and this is actually going to look like a
	 * nested function call in user code. Thus this overload makes it less
	 * work for you as you don't need to know whether the generator is
	 * going to create a single engine or a series of engines (which is to
	 * say, whether it's going to have everything run concurrently or have
	 * a number of sets of concurrent calculations.) */

	void add_worksets(const engine_t& e);
	void add_worksets(const std::list<engine_t>& e);

	/* We're gonna have custom constructors, one without arguments, for
	 * using all available threads in the system, and another for only
	 * using a fixed number of compute threads */

	sandbox_t();
	sandbox_t(size_t concurrency);

	~sandbox_t(); // We need a custom destructor to clean up the engines.

	std::thread main_thread{};
	bool finished = false; // Used for signalling.

	void start(); // Used for starting and stopping the simulation.
	void stop();

	/* These functions finds entries in a sandbox_t and return them, but if
 	 * they don't exist, they generate them using the default values
 	 * specified above. */

	data_t& config_get(const std::string& id);
	data_vector_t& database_get(const std::string& id);
};

}
#endif