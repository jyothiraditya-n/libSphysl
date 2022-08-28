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

#include <cinttypes>
#include <cmath>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <tuple>

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

	#include <signal.h>
	#include <termios.h>
	#include <unistd.h>

	#include <LC_args.h>
	#include <LC_vars.h>

	#include <LSC_buffer.h>
	#include <LSC_error.h>
	#include <LSC_lines.h>
}

using namespace std::chrono;
using namespace std;

using namespace libSphysl::logging;
using namespace libSphysl::motion;
using namespace libSphysl::time;
using namespace libSphysl::utility;
using namespace libSphysl;

const char *name;
char output[4096] = "";
bool running = true;

sandbox_t sandbox;
LSCb_t buffer;

struct termios cooked, raw;
size_t height, width;

bool colour = false;
bool clean = false;

size_t entities = 100;
size_t log_freq = 1;
size_t exec_time = 0;
size_t step_time = 1;

double side_length = 1.0;

double min_velocity = -1.0;
double max_velocity = 1.0;

double min_part_size = 0.0;
double max_part_size = 0.0;

double min_part_mass = 0.0;
double max_part_mass = 0.0;

void about();
void help(int ret);

void help_flag();
void init_flags(int argc, char **argv);

void on_interrupt(int signum);
void renderer(sandbox_t* s, void* arg);

int main(int argc, char **argv) {
	name = argv[0];
	init_flags(argc, argv);

	if(min_part_size == 0.0) {
		min_part_size = side_length / pow(entities, 1.0 / 3.0);
		min_part_size -= min_part_size / 5.0;
	}

	if(max_part_size == 0.0) {
		max_part_size = side_length / pow(entities, 1.0 / 3.0);
		max_part_size += max_part_size / 5.0;
	}

	if(min_part_mass == 0.0) {
		min_part_size = 1.204 * pow(side_length, 3.0) / entities;
		min_part_size -= min_part_mass / 5.0;
	}

	if(max_part_mass == 0.0) {
		max_part_mass = 1.204 * pow(side_length, 3.0) / entities;
		max_part_mass += max_part_mass / 5.0;
	}

	sandbox.config["entity count"] = size_t{entities + 1};

	sandbox.add_engine(collision::box(&sandbox));
	sandbox.config["bounding box width"] = side_length;
	sandbox.config["bounding box height"] = side_length;
	sandbox.config["bounding box depth"] = side_length;

	randomise(
		sandbox.database["bounding box width"],
		min_part_size, max_part_size
	);

	randomise(
		sandbox.database["bounding box height"],
		min_part_size, max_part_size
	);

	randomise(
		sandbox.database["bounding box depth"],
		min_part_size, max_part_size
	);

	sandbox.add_engine(classical(&sandbox));

	randomise(
		sandbox.database["x position"],
		-side_length, side_length
	);

	randomise(
		sandbox.database["y position"],
		-side_length, side_length
	);
	
	randomise(
		sandbox.database["z position"],
		-side_length, side_length
	);

	randomise(
		sandbox.database["x velocity"],
		min_velocity, max_velocity
	);

	randomise(
		sandbox.database["y velocity"],
		min_velocity, max_velocity
	);

	randomise(
		sandbox.database["z velocity"],
		min_velocity, max_velocity
	);

	randomise(
		sandbox.database["mass"],
		min_part_mass, max_part_mass
	);

	sandbox.add_engine(constant(&sandbox));
	sandbox.config["time change"] = step_time * pow(10.0, -6.0);

	signal(SIGINT, on_interrupt);

	int ret = tcgetattr(STDIN_FILENO, &cooked);
	if(ret == -1) {
		puts("Error getting terminal properties with tcgetattr().");
		exit(1);
	}

	raw = cooked;
	raw.c_lflag &= ~(ICANON | ECHO);

	ret = tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	if(ret == -1) {
		puts("Error setting terminal properties with tcsetattr().");
		exit(2);
	}

	printf("\033[999;999H\033[6n");
	while(getchar() != '\033');

	ret = scanf("[%zu;%zuR", &height, &width);
	if(ret != 2) {
		tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		puts("Error getting screen size with ANSI escape codes.");
		exit(3);
	}

	printf("\033[?25l");
	LSCb_init(&buffer);

	buffer.height = height;
	buffer.width = width;
	buffer.colour = colour;

	buffer.validate = true;
	buffer.cchs = "\033[48;5;011m\033[38;5;015m ";

	ret = LSCb_alloc(&buffer);
	if(ret != LSCE_OK) {
		tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		puts("Error initialising libScricon.");
		printf("\033[?25h");
		exit(4);
	}

	if(strlen(output) && strcmp(output, "-")) {
		sandbox.add_engine(csv(
			&sandbox, output, log_freq, 10,
			{"x position", "y position", "z position"},
			{"time", "time change"}
		));
	}

	sandbox.add_engine(
		engine_t{renderer, null_destructor, &sandbox, {NULL}}
	);

	sandbox.start();
	
	if(exec_time) {
		this_thread::sleep_for(seconds(exec_time));
	}

	else while(running) {
		this_thread::sleep_for(seconds(1));
	}

	sandbox.stop();
	tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	printf("\033[?25h");
	fflush(stdout);
	return 0;
}

void about() {
	putchar('\n');
	puts("  The Sphysl Project Copyright (C) 2022 Jyothiraditya Nellakra");
	puts("  Brownian Motion Demonstration\n");

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

void help(int ret) {
	putchar('\n');
	printf("  Usage: %s [OPTIONS]\n\n", name);

	puts("  Valid options are:");
	puts("    -a, --about               print the about dialogue");
	puts("    -h, --help                print this help dialogue\n");

	puts("    -c, --colour              enable ANSI colour support");
	puts("    -C, --clean               don't draw secondary particle velocities.\n");

	puts("    -e, --entities NUM        set the number of entities");
	puts("    -t, --step-time USECS     set the time per simulation step");
	puts("    -T, --exec-time SECS      set the execution time\n");

	puts("    -f, --log-freq N          set the logging to every N cycles");
	puts("    -o, --output FILE         set the output file\n");

	puts("    -s, --side-length METRES  set the side length of the container");
	puts("    -p, --min-part-size RAD   set the minimum particle radius");
	puts("    -P, --max-part-size RAD   set the maximum particle radius\n");

	puts("    -v, --min-velocity M/S    set the minimum random particle velocity");
	puts("    -V, --max-velocity M/S    set the maximum random particle velocity\n");

	puts("    -m, --min-part-mass KG    set the minimum particle mass");
	puts("    -M, --max-part-mass KG    set the maximum particle mass\n");

	puts("  Happy coding! :)\n");
	exit(ret);
}

void help_flag() {
	help(0);
}

void init_flags(int argc, char **argv) {
	LCa_t *arg = LCa_new(); arg -> long_flag = "about";
	arg -> short_flag = 'a'; arg -> pre = about;

	arg = LCa_new(); arg -> long_flag = "help";
	arg -> short_flag = 'h'; arg -> pre = help_flag;

	LCv_t *var = LCv_new(); var -> id = "colour"; var -> data = &colour;
	arg = LCa_new(); arg -> long_flag = "colour"; arg -> short_flag = 'c';
	arg -> var = var; arg -> value = true;

	var = LCv_new(); var -> id = "clean"; var -> data = &clean;
	arg = LCa_new(); arg -> long_flag = "clean"; arg -> short_flag = 'C';
	arg -> var = var; arg -> value = true;

	var = LCv_new(); var -> id = "output";
	var -> fmt = "%4095c"; var -> data = output;
	arg = LCa_new(); arg -> long_flag = "output";
	arg -> short_flag = 'o'; arg -> var = var;

	var = LCv_new(); var -> id = "entities";
	var -> fmt = "%zu"; var -> data = &entities;
	arg = LCa_new(); arg -> long_flag = "entities";
	arg -> short_flag = 'e'; arg -> var = var;

	var = LCv_new(); var -> id = "side_length";
	var -> fmt = "%lf"; var -> data = &side_length;
	arg = LCa_new(); arg -> long_flag = "side-length";
	arg -> short_flag = 's'; arg -> var = var;

	var = LCv_new(); var -> id = "log_freq";
	var -> fmt = "%zu"; var -> data = &log_freq;
	arg = LCa_new(); arg -> long_flag = "log-freq";
	arg -> short_flag = 'f'; arg -> var = var;

	var = LCv_new(); var -> id = "step_time";
	var -> fmt = "%zu"; var -> data = &step_time;
	arg = LCa_new(); arg -> long_flag = "step-time";
	arg -> short_flag = 't'; arg -> var = var;

	var = LCv_new(); var -> id = "exec_time";
	var -> fmt = "%zu"; var -> data = &exec_time;
	arg = LCa_new(); arg -> long_flag = "exec-time";
	arg -> short_flag = 'T'; arg -> var = var;

	var = LCv_new(); var -> id = "min_velocity";
	var -> fmt = "%lf"; var -> data = &min_velocity;
	arg = LCa_new(); arg -> long_flag = "min-velocity";
	arg -> short_flag = 'v'; arg -> var = var;

	var = LCv_new(); var -> id = "max_velocity";
	var -> fmt = "%lf"; var -> data = &max_velocity;
	arg = LCa_new(); arg -> long_flag = "max-velocity";
	arg -> short_flag = 'V'; arg -> var = var;

	var = LCv_new(); var -> id = "min_part_size";
	var -> fmt = "%lf"; var -> data = &min_part_size;
	arg = LCa_new(); arg -> long_flag = "min-part-size";
	arg -> short_flag = 'p'; arg -> var = var;

	var = LCv_new(); var -> id = "max_part_size";
	var -> fmt = "%lf"; var -> data = &max_part_size;
	arg = LCa_new(); arg -> long_flag = "max-part-size";
	arg -> short_flag = 'P'; arg -> var = var;

	var = LCv_new(); var -> id = "min_part_mass";
	var -> fmt = "%lf"; var -> data = &min_part_mass;
	arg = LCa_new(); arg -> long_flag = "min-part-mass";
	arg -> short_flag = 'm'; arg -> var = var;

	var = LCv_new(); var -> id = "max_part_mass";
	var -> fmt = "%lf"; var -> data = &max_part_mass;
	arg = LCa_new(); arg -> long_flag = "max-part-mass";
	arg -> short_flag = 'M'; arg -> var = var;

	int ret = LCa_read(argc, argv);
	if(ret != LCA_OK) help(1);
}

void renderer(sandbox_t* s, void* arg) {
	static auto& tick = get<size_t>(s -> config["simulation tick"]);
	if(tick % log_freq) return;

	static auto& x2s = sandbox.database.at("x position");
	static auto& y2s = sandbox.database.at("y position");
	static auto& z2s = sandbox.database.at("z position");
	static auto x1s{x2s}, y1s{y2s}, z1s{z2s};

	static auto& x2 = get<double>(x2s[0]);
	static auto& y2 = get<double>(y2s[0]);
	static auto& z2 = get<double>(z2s[0]);
	static double x1{x2}, y1{y2}, z1{z2};

	static auto lines = list{tuple{x1, y1, z1, x2, y2, z2}};
	(void) arg;
	(void) s;

	LSCb_clear(&buffer);

	auto draw_others = [&](){
		auto y1it = y1s.begin(), z1it = z1s.begin();
		auto x2it = x2s.begin(), y2it = y2s.begin(), z2it = z2s.begin();
		uint8_t c = 17;

		for(const auto& i: x1s) {
			const auto x1 = get<double>(i);
			const auto y1 = get<double>(*y1it);
			const auto z1 = get<double>(*z1it);

			const auto x2 = get<double>(*x2it);
			const auto y2 = get<double>(*y2it);
			const auto z2 = get<double>(*z2it);

			if(c > 230) break;

			if(colour) LSCl_drawfg(&buffer,
				LSCb_getxz(&buffer,
					x1 / side_length, z1 - side_length
				),

				LSCb_getyz(&buffer,
					y1 / side_length, z1 - side_length
				),

				LSCb_getxz(&buffer,
					x2 / side_length, z2 - side_length
				),

				LSCb_getyz(&buffer,
					y2 / side_length, z2 - side_length
				),

				c
			);

			else LSCl_set(&buffer,
				LSCb_getxz(&buffer,
					x1 / side_length, z1 - side_length
				),

				LSCb_getyz(&buffer,
					y1 / side_length, z1 - side_length
				),

				LSCb_getxz(&buffer,
					x2 / side_length, z2 - side_length
				),

				LSCb_getyz(&buffer,
					y2 / side_length, z2 - side_length
				),

				'.'
			);

			LSCb_setzv(&buffer,
				LSCb_getxz(&buffer,
					x2 / side_length, z2 - side_length
				),

				LSCb_getyz(&buffer,
					y2 / side_length, z2 - side_length
				),

				z2 - side_length, 'o'
			);

			advance(y1it, 1); advance(z1it, 1);
			advance(x2it, 1); advance(y2it, 1); advance(z2it, 1);

			c++;
		}
	};

	if(!clean) draw_others();
	uint8_t c = 255;

	for(const auto& i: lines) {
		if(colour) LSCl_setbgz(&buffer,
			LSCb_getxz(&buffer,
				get<0>(i) / side_length,
				get<2>(i) - side_length
			),

			LSCb_getyz(&buffer,
				get<1>(i) / side_length,
				get<2>(i) - side_length
			),

			get<2>(i) - side_length,

			LSCb_getxz(&buffer,
				get<3>(i) / side_length,
				get<5>(i) - side_length
			),

			LSCb_getyz(&buffer,
				get<4>(i) / side_length,
				get<5>(i) - side_length
			),

			get<5>(i) - side_length, c
		);

		else LSCl_drawz(&buffer,
			LSCb_getxz(&buffer,
				get<0>(i) / side_length,
				get<2>(i) - side_length
			),

			LSCb_getyz(&buffer,
				get<1>(i) / side_length,
				get<2>(i) - side_length
			),

			get<2>(i) - side_length,

			LSCb_getxz(&buffer,
				get<3>(i) / side_length,
				get<5>(i) - side_length
			),

			LSCb_getyz(&buffer,
				get<4>(i) / side_length,
				get<5>(i) - side_length
			),

			get<5>(i) - side_length
		);

		c--;
	}

	LSCb_print(&buffer, 1);

	lines.push_back({x1, y1, z1, x2, y2, z2});
	if(colour && lines.size() > 24) lines.pop_front();

	x1 = x2; y1 = y2; z1 = z2;
	x1s = x2s; y1s = y2s; z1s = z2s;
}

void on_interrupt(int signum) {
	if(signum != SIGINT) {
		signal(signum, SIG_DFL);
		return;
	}

	running = false;
}