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

/* This function is going to be used to print the simulation time info, the
 * velocity, acceleration, kinetic and total energy of the electron, and the
 * magnitude of the force acting to constantly accelerate it. */
void display(void *arg);

/* This function is going to be used to simulate an electric field that
 * accelerates the electron with a constant force. */
void field(void *arg);

/* Function Definitons */

int main() {
	/* Our sandbox has one entity: the mass on the spring. */
	sandbox.config["entity count"] = std::size_t{1};

	/* Add a constant clock to the sandbox, one tick = 500 ns. */
	sandbox.add_worksets(libSphysl::time::constant(&sandbox));
	sandbox.config["time change"] = 5.0 * std::pow(10.0, -7.0); // seconds.

	/* Add an unsmoothed relativistic motion engine to the sandbox. */
	sandbox.add_worksets(libSphysl::motion::relativistic(&sandbox));

	/* Set the mass of the electron to be the literature value. We have to
	 * mess around with vectors because this is technically the 0th object
	 * in a system of 1 object(s). */
	std::get<std::vector<double>>(sandbox.database_get("mass"))[0]
		= 9.10938188 * std::pow(10.0, -31.0); // kilogrammes.

	/* Create an engine for the display function and add it to the
	 * sandbox as a workset. */
	libSphysl::engine_t engine;
	engine.calculator = display;
	engine.args.push_back(NULL); // Dummy argument to have a calculation.
	engine.destructor = libSphysl::utility::null_destructor;
	sandbox.add_worksets(engine);

	/* Modify the engine to work for the field and add it to the sandbox
	 * as a workset as well. */
	engine.calculator = field;
	sandbox.add_worksets(engine);

	/* Clear std::cout and configure it to output doubles in the way we
	 * want. This usse ANSI escape codes, see endnote [1] for more. */
	std::cout << "\033[2J" << std::fixed << std::setprecision(2);

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
		sandbox.config_get("time")
	);

	static const auto& c = std::get<double>(
		sandbox.config_get("speed of light")
	);

	/* For the velocity and the acceleration, we only use the x-axis of the
	 * simulation system. (Note: We are getting a reference to the first
	 * and only entitie's values, hence the vector element access.) */

	static const auto& v = std::get<std::vector<double>>(
		sandbox.database_get("x velocity")
	)[0];

	static const auto& a = std::get<std::vector<double>>(
		sandbox.database_get("x acceleration")
	)[0];

	static const auto& m = std::get<std::vector<double>>(
		sandbox.database_get("mass")
	)[0];

	/* Gamma = 1 / sqrt(1 - (v/c)^2) = 1 / sqrt(1 - v^2 / c^2). */
	const auto gamma = 1.0 / std::sqrt(1.0 - (v * v) / (c * c));

	/* Conversion factor for units. */
	const auto J = 6.241509 * std::pow(10.0, 12.0); // mega-electron-Volts.

	const auto KE = gamma * m * c * c; // KE = gamma m c^2.
	const auto PE = 0.51099895000; // mega-electron-Volts.

	/* Clear the screen and move the cursor back to the home position
	 * before printing the variables with their names and units. This uses
	 * ANSI escape codes, see endnote [1] for more details on that. */
	std::cout << "\033[H\033[?25l"
		<< "Simulation Time: \033[0K" << t << " s\n\n"
		<< "Velocity:     \033[0K" << v / c << " c\n"
		<< "Acceleration: \033[0K" << a / c << " c/s\n\n"
		<< "Kinetic Energy:   \033[0K" << KE * J << " MeV\n"
		<< "Potential Energy: \033[0K" << PE << " MeV\n"
		<< "Total Energy:     \033[0K" << KE * J + PE << " MeV\n\n"
		<< "Electric Field: 1.00 N/C"
		<< "\033[?25h" << std::endl;
}

void field(void *arg) {
	(void) arg; // We don't actually need this default variable here.

	/* For the force, we only use the x-axis of the simulation system.
	 * (Note: We are getting a reference to the first and only entitie's
	 * values, hence the vector element access.) */

	static auto& F = std::get<std::vector<double>>(
		sandbox.database_get("x force")
	)[0];

	const auto q = 1.602176634 * std::pow(10.0, -19.0); // Coulombs
	const auto E = 1.0; // Newtons / Coulombs

	/* Electric Force; F = qE. */
	F = q * E;
}

/* [1] <https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797> */