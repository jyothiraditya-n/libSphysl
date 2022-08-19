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
#include <fstream>
#include <iostream>
#include <thread>

#include <libSphysl.h>
#include <libSphysl/gravity.h>
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
}

using namespace std::chrono;
using namespace std;

using namespace libSphysl::logging;
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

size_t entities = 255;
size_t log_freq = 1;
size_t exec_time = 0;
size_t step_time = 1;

void about();
void help(int ret);

void help_flag();
void init_flags(int argc, char **argv);

void on_interrupt(int signum);
void renderer(sandbox_t* s, void* arg);

int main(int argc, char **argv) {
	name = argv[0];
	init_flags(argc, argv);

	sandbox.config["entity count"] = size_t{10};

	sandbox.add_engine(gravity::classical(&sandbox));
	sandbox.add_engine(motion::classical(&sandbox));

	auto& xs = sandbox.database.at("x position");
	xs[1] = 6.0 * std::pow(10.0, 10.0);
	xs[2] = -1.0 * std::pow(10.0, 11.0);
	xs[3] = 1.5 * std::pow(10.0, 11.0);
	xs[4] = -2.2 * std::pow(10.0, 11.0);
	xs[5] = 7.8 * std::pow(10.0, 11.0);
	xs[6] = -1.4 * std::pow(10.0, 12.0);
	xs[7] = 2.9 * std::pow(10.0, 12.0);
	xs[8] = -4.5 * std::pow(10.0, 12.0);
	xs[9] = 6.3 * std::pow(10.0, 12.0);

	auto& v_ys = sandbox.database.at("y velocity");
	v_ys[1] = 4.7 * std::pow(10.0, 4.0);
	v_ys[2] = -3.5 * std::pow(10.0, 4.0);
	v_ys[3] = 3.0 * std::pow(10.0, 4.0);
	v_ys[4] = -2.4 * std::pow(10.0, 4.0);
	v_ys[5] = 1.3 * std::pow(10.0, 4.0);
	v_ys[6] = -9.7 * std::pow(10.0, 3.0);
	v_ys[7] = 6.8 * std::pow(10.0, 3.0);
	v_ys[8] = -5.4 * std::pow(10.0, 3.0);
	v_ys[9] = 4.7 * std::pow(10.0, 3.0);

	auto& ms = sandbox.database.at("mass");
	ms[0] = 2.0 * std::pow(10.0, 30.0);
	ms[1] = 6.1 * std::pow(10.0, 10.0);
	ms[2] = 4.9 * std::pow(10.0, 24.0);
	ms[3] = 6.0 * std::pow(10.0, 24.0);
	ms[4] = 6.4 * std::pow(10.0, 23.0);
	ms[5] = 1.9 * std::pow(10.0, 27.0);
	ms[6] = 5.7 * std::pow(10.0, 26.0);
	ms[7] = 8.7 * std::pow(10.0, 25.0);
	ms[8] = 1.0 * std::pow(10.0, 26.0);
	ms[9] = 1.3 * std::pow(10.0, 22.0);

	sandbox.add_engine(constant(&sandbox));
	sandbox.config.at("time change") = step_time * 24.0 * 3600.0;

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
			{"x acceleration", "y acceleration", "z acceleration"},
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
	puts("  Solar System Demonstration\n");

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

	puts("    -e, --entities NUM        set the number of entities");
	puts("    -t, --step-time DAYS      set the time per simulation step");
	puts("    -T, --exec-time SECS      set the execution time\n");

	puts("    -f, --log-freq N          set the logging to every N cycles");
	puts("    -o, --output FILE         set the output file\n");

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

	LCv_t *var = LCv_new(); var -> id = "output";
	var -> fmt = "%4095c"; var -> data = output;
	arg = LCa_new(); arg -> long_flag = "output";
	arg -> short_flag = 'o'; arg -> var = var;

	var = LCv_new(); var -> id = "entities";
	var -> fmt = "%zu"; var -> data = &entities;
	arg = LCa_new(); arg -> long_flag = "entities";
	arg -> short_flag = 'e'; arg -> var = var;

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

	int ret = LCa_read(argc, argv);
	if(ret != LCA_OK) help(1);
}

void renderer(sandbox_t* s, void* arg) {
	auto& xs = sandbox.database.at("x position");
	auto& ys = sandbox.database.at("y position");
	(void) arg;
	(void) s;

	auto min_x = 0.0, max_x = 0.0;
	auto min_y = 0.0, max_y = 0.0;

	for(const auto& i: xs) {
		const auto x = get<double>(i);
		if(x < min_x) min_x = x;
		if(x > max_x) max_x = x;
	}

	for(const auto& i: ys) {
		const auto y = get<double>(i);
		if(y < min_y) min_y = y;
		if(y > max_y) max_y = y;
	}

	LSCb_clear(&buffer);
	auto delta_x = max_x - min_x;
	auto delta_y = max_y - min_y;

	if(delta_x == 0.0) delta_x = 1.0;
	if(delta_y == 0.0) delta_y = 1.0;

	if(delta_x < delta_y) delta_x = delta_y;
	else delta_y = delta_x;

	auto it = xs.begin();
	size_t ind = 0;
	for(const auto& i: ys) {
		const auto x = get<double>(*it);
		const auto y = get<double>(i);
		
		LSCb_set(&buffer,
			LSCb_getx(&buffer, 2.0 * (x - min_x) / delta_x - 1.0),
			LSCb_gety(&buffer, 2.0 * (y - min_y) / delta_y - 1.0),
			'0' + ind
		);

		advance(it, 1);
		ind++;
	}

	LSCb_print(&buffer, 1);
}

void on_interrupt(int signum) {
	if(signum != SIGINT) {
		signal(signum, SIG_DFL);
		return;
	}

	running = false;
}