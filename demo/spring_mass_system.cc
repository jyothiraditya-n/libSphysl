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

#include <iomanip>
#include <iostream>

/* Including Library Headerfiles */

#include <libSphysl.h>
#include <libSphysl/motion.h>
#include <libSphysl/time.h>
#include <libSphysl/utility.h>

/* Variable Declarations */

/* Create a sandbox with only one thread. */
libSphysl::sandbox_t sandbox(1);

/* Function Declarations */

/* This function is going to be used to print the simulation time, the
 * displacement, velocity and kinetic energy of the mass on the spring and the
 * kinetic energy of the overall system. */
void display(void *arg);

/* This function is going to be used to apply a Force on the mass equivalent
 * to a spring returning it to the origin. */
void spring(void *arg);

/* Function Definitons */

int main() {
	/* Our sandbox has one entity: the mass on the spring. */
	sandbox.config["entity count"] = std::size_t{1};

	/* Add a realtime clock to the sandbox. */
	sandbox.add_worksets(libSphysl::time::realtime(&sandbox));

	/* Add an unsmoothed classical motion engine to the sandbox. */
	sandbox.add_worksets(libSphysl::motion::classical(&sandbox));

	/* Set the initial displacement of the mass to be -1 m. We are only
	 * using the x-axis of the simulation in this demonstration. We have to
	 * mess around with vectors because this is technically the 0th object
	 * in a system of 1 object(s). */
	std::get<std::vector<double>>(sandbox.database_get("x position"))[0]
		= -1.0;

	/* Notably, the mass is already set to 1 Kg by default. */

	/* Create an engine for the display function and add it to the
	 * sandbox as a workset. */
	libSphysl::engine_t engine;
	engine.calculator = display;
	engine.args.push_back(NULL); // Dummy argument to have a calculation.
	engine.destructor = libSphysl::utility::null_destructor;
	sandbox.add_worksets(engine);

	/* Modify the engine to work for the spring and add it to the sandbox
	 * as a workset as well. */
	engine.calculator = spring;
	sandbox.add_worksets(engine);

	/* Clear std::cout and configure it to output doubles in the way we
	 * want. This usse ANSI escape codes, see endnote [1] for more. */
	std::cout << "\033[2J" << std::scientific << std::setprecision(2);

	/* Start the sandbox and loop forever. C++ will handle exiting and
	 * cleanup when the user hits ^C to stop program execution.. */
	sandbox.start();
	while(true);

	/* This will never be executed, but it makes the compiler happy. */
	return 0;
}

void display(void *arg) {
	(void) arg; // We don't actually need this default variable here.

	/* Cache references to the variables we use. */
	static const auto& t = std::get<double>(
		sandbox.config.at("time")
	);

	/* For the displacement and the velocity, we only use the x-axis of the
	 * simulation system. (Note: We are getting a reference to the first
	 * and only entitie's values, hence the vector element access.) */

	static const auto& x = std::get<std::vector<double>>(
		sandbox.database_get("x position")
	)[0];

	static const auto& v = std::get<std::vector<double>>(
		sandbox.database_get("x velocity")
	)[0];

	const auto KE = 0.5 * 1.0 * v * v; // KE = 0.5 m v^2.
	const auto PE = 0.5 * 1.0 * x * x; // PE = 0.5 k x^2.

	/* Clear the screen and move the cursor back to the home position
	 * before printing the variables with their names and units. This uses
	 * ANSI escape codes, see endnote [1] for more details on that. */
	std::cout << "\033[H"
		<< "Simulation Time: \033[0K" << t << " seconds\n"
		<< "Displacement: \033[0K" << x << " metres\n"
		<< "Velocity: \033[0K" << v << " metres / second\n"
		<< "Kinetic Energy: \033[0K" << KE << " Joules\n"
		<< "Potential Energy: \033[0K" << PE << " Joules\n"
		<< std::endl;
}

void spring(void *arg) {
	(void) arg; // We don't actually need this default variable here.

	/* Spring constant. */
	const auto k = 1.0; // Newtons / metre

	/* For the displacement and the velocity, we only use the x-axis of the
	 * simulation system. (Note: We are getting a reference to the first
	 * and only entitie's values, hence the vector element access.) */

	static const auto& x = std::get<std::vector<double>>(
		sandbox.database_get("x position")
	)[0];

	static auto& F = std::get<std::vector<double>>(
		sandbox.database_get("x force")
	)[0];

	/* Spring force equation. */
	F = - k * x;
}

/* [1] <https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797> */