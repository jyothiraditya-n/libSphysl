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

#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

#include <libSphysl.h>
#include <libSphysl/collision.h>
#include <libSphysl/logging.h>
#include <libSphysl/motion.h>
#include <libSphysl/time.h>
#include <libSphysl/utility.h>

extern "C" {
	#include <stdbool.h>
	#include <stddef.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#include <LC_args.h>
	#include <LC_vars.h>
}

static const char *name;
static char output[4096] = "";

static size_t entities = 100;
static size_t frequency = 1000;
static size_t exec_time = 0;

static double side_length = 10.0;

static double min_velocity = 0.0;
static double max_velocity = 1.0;

static double min_part_size = 0.0;
static double max_part_size = 1.0;

static double min_part_mass = 1.0;
static double max_part_mass = 1.0;

static void about();
static void help(int ret);

static void help_flag();
static void init_flags(int argc, char **argv);

int main(int argc, char **argv) {
	name = argv[0];
	init_flags(argc, argv);

	libSphysl::sandbox_t sandbox;
	sandbox.config["entity count"] = std::size_t{entities + 1};

	sandbox.add_engine(libSphysl::collision::box(&sandbox));
	sandbox.config["bounding box width"] = side_length;
	sandbox.config["bounding box height"] = side_length;
	sandbox.config["bounding box depth"] = side_length;

	libSphysl::utility::randomise(
		sandbox.database["bounding box width"],
		min_part_size, max_part_size
	);

	libSphysl::utility::randomise(
		sandbox.database["bounding box height"],
		min_part_size, max_part_size
	);

	libSphysl::utility::randomise(
		sandbox.database["bounding box depth"],
		min_part_size, max_part_size
	);

	std::ofstream file;
	if(!std::strlen(output) && std::strcmp(output, "-")) sandbox.add_engine(
		libSphysl::logging::csv(&sandbox, std::cout, frequency, 1,
		{"x position", "y position", "z position"},
		{"time", "time change"}
	));

	else {
		file = std::ofstream{output};
		sandbox.add_engine(
			libSphysl::logging::csv(&sandbox, file, frequency, 1,
			{"x position", "y position", "z position"},
			{"time", "time change"}
		));
	}

	sandbox.add_engine(libSphysl::motion::classical(&sandbox));

	libSphysl::utility::randomise(
		sandbox.database["x position"], -side_length, side_length
	);

	libSphysl::utility::randomise(
		sandbox.database["y position"], -side_length, side_length
	);
	
	libSphysl::utility::randomise(
		sandbox.database["z position"], -side_length, side_length
	);

	libSphysl::utility::randomise(
		sandbox.database["x velocity"], min_velocity, max_velocity
	);

	libSphysl::utility::randomise(
		sandbox.database["y velocity"], min_velocity, max_velocity
	);

	libSphysl::utility::randomise(
		sandbox.database["z velocity"], min_velocity, max_velocity
	);

	libSphysl::utility::randomise(
		sandbox.database["mass"], min_part_mass, max_part_mass
	);

	sandbox.add_engine(libSphysl::time::realtime(&sandbox));
	sandbox.config["minimum time change"] = 5.0 * std::pow(10.0, -7.0);
	sandbox.config["maximum time change"] = 4.9 * std::pow(10.0, -6.0);

	sandbox.start();
	
	if(exec_time) {
		std::this_thread::sleep_for(std::chrono::seconds(exec_time));
	}

	else while(true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	sandbox.stop();
	return 0;
}

static void about() {
	putchar('\n');
	puts("  The Sphysl Project Copyright (C) 2022 Jyothiraditya Nellakra\n");

	puts("  This program is free software: you can redistribute it and/or modify");
	puts("  it under the terms of the GNU General Public License as published by");
	puts("  the Free Software Foundation, either version 3 of the License, or");
	puts("  (at your option) any later version.\n");

	puts("  This program is distributed in the hope that it will be useful,");
	puts("  but WITHOUT ANY WARRANTY; without even the implied warranty of");
	puts("  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the");
	puts("  GNU General Public License for more details.\n");

	puts("  You should have received a copy of the GNU General Public License");
	puts("  along with this program. If not, see <https://www.gnu.org/licenses/>.\n");

	exit(0);
}

static void help(int ret) {
	putchar('\n');
	printf("  Usage: %s [OPTIONS] [--] [FILES]\n\n", name);

	puts("  Valid options are:");
	puts("    -a, --about               print the about dialogue");
	puts("    -h, --help                print this help dialogue\n");

	puts("    -e, --entities NUM        set the number of entities");
	puts("    -f, --frequency N         set the logging to every N cycles");
	puts("    -o, --output FILE         set the output file\n");
	puts("    -t, --time SECONDS        set the execution time.\n");

	puts("    -s, --side-length LEN     set the side length of the container");
	puts("    -v, --min-velocity VEL    set the minimum random particle velocity");
	puts("    -V, --max-velocity VEL    set the maximum random particle velocity");
	puts("    -p, --min-part-size RAD   set the minimum particle radius");
	puts("    -P, --max-part-size RAD   set the maximum particle radius");
	puts("    -m, --min-part-mass MASS  set the minimum particle mass");
	puts("    -M, --max-part-mass MASS  set the maximum particle mass");

	puts("  Happy coding! :)\n");
	exit(ret);
}

static void help_flag() {
	help(0);
}

static void init_flags(int argc, char **argv) {
	LCa_t *arg = LCa_new();
	arg -> long_flag = "about";
	arg -> short_flag = 'a';
	arg -> pre = about;

	arg = LCa_new();
	arg -> long_flag = "help";
	arg -> short_flag = 'h';
	arg -> pre = help_flag;

	LCv_t *var = LCv_new();
	var -> id = "output";
	var -> fmt = "%4095c";
	var -> data = output;

	arg = LCa_new();
	arg -> long_flag = "output";
	arg -> short_flag = 'o';
	arg -> var = var;

	var = LCv_new();
	var -> id = "entities";
	var -> fmt = "%zu";
	var -> data = &entities;

	arg = LCa_new();
	arg -> long_flag = "entities";
	arg -> short_flag = 'e';
	arg -> var = var;

	var = LCv_new();
	var -> id = "side_length";
	var -> fmt = "%lf";
	var -> data = &side_length;

	arg = LCa_new();
	arg -> long_flag = "side-length";
	arg -> short_flag = 's';
	arg -> var = var;

	var = LCv_new();
	var -> id = "frequency";
	var -> fmt = "%zu";
	var -> data = &frequency;

	arg = LCa_new();
	arg -> long_flag = "frequency";
	arg -> short_flag = 'f';
	arg -> var = var;

	var = LCv_new();
	var -> id = "time";
	var -> fmt = "%zu";
	var -> data = &exec_time;

	arg = LCa_new();
	arg -> long_flag = "time";
	arg -> short_flag = 't';
	arg -> var = var;

	var = LCv_new();
	var -> id = "min_velocity";
	var -> fmt = "%lf";
	var -> data = &min_velocity;

	arg = LCa_new();
	arg -> long_flag = "min-velocity";
	arg -> short_flag = 'v';
	arg -> var = var;

	var = LCv_new();
	var -> id = "max_velocity";
	var -> fmt = "%lf";
	var -> data = &max_velocity;

	arg = LCa_new();
	arg -> long_flag = "max-velocity";
	arg -> short_flag = 'V';
	arg -> var = var;

	var = LCv_new();
	var -> id = "min_part_size";
	var -> fmt = "%lf";
	var -> data = &min_part_size;

	arg = LCa_new();
	arg -> long_flag = "min-part-size";
	arg -> short_flag = 'p';
	arg -> var = var;

	var = LCv_new();
	var -> id = "max_part_size";
	var -> fmt = "%lf";
	var -> data = &max_part_size;

	arg = LCa_new();
	arg -> long_flag = "max-part-size";
	arg -> short_flag = 'P';
	arg -> var = var;

	var = LCv_new();
	var -> id = "min_part_mass";
	var -> fmt = "%lf";
	var -> data = &min_part_mass;

	arg = LCa_new();
	arg -> long_flag = "min-part-mass";
	arg -> short_flag = 'm';
	arg -> var = var;

	var = LCv_new();
	var -> id = "max_part_mass";
	var -> fmt = "%lf";
	var -> data = &max_part_mass;

	arg = LCa_new();
	arg -> long_flag = "max-part-mass";
	arg -> short_flag = 'M';
	arg -> var = var;

	int ret = LCa_read(argc, argv);
	if(ret != LCA_OK) help(1);
}