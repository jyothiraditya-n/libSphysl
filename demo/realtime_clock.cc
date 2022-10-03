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
#include <libSphysl/time.h>
#include <libSphysl/utility.h>

/* Variable Declarations */

/* Create a sandbox with only one thread. */
libSphysl::sandbox_t sandbox(1);

/* Function Declarations */

/* This function is going to be used to print the simulation tick, real-world
 * time change since simulation start, and time change per simulation tick. */
void display(void *arg);

/* Function Definitons */

int main() {
	/* Add a realtime clock to the sandbox. */
	sandbox.add_worksets(libSphysl::time::realtime(&sandbox));

	/* Create an engine for the display function and add it to the
	 * sandbox as a workset. */
	libSphysl::engine_t engine;
	engine.calculator = display;
	engine.args.push_back(NULL); // Dummy argument to have a calculation.
	engine.destructor = libSphysl::utility::null_destructor;
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
	static const auto& time = std::get<double>(
		sandbox.config_get("time")
	);

	static const auto& tick = std::get<size_t>(
		sandbox.config_get("simulation tick")
	);

	static const auto& delta_t = std::get<double>(
		sandbox.config_get("time change")
	);

	/* Clear the screen and move the cursor back to the home position
	 * before printing the variables with their names and units. This uses
	 * ANSI escape codes, see endnote [1] for more details on that. */
	std::cout << "\033[H"
		<< "Simulation Time: \033[0K" << time << " seconds\n"
		<< "Time Change: \033[0K" << delta_t << " seconds\n"
		<< "Simulation Tick: \033[0K" << tick
		<< std::endl;
}

/* [1] <https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797> */