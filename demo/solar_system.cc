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

#include <cmath>

#include <chrono>
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
	#include <LSC_lines.h>
}

using namespace std::chrono;
using namespace std;

using namespace libSphysl::utility;
using namespace libSphysl;

const char *name;
char output[4096] = "";
bool running = true;

sandbox_t sandbox;
LSCb_t buffer;

struct termios cooked, raw;
size_t height, width;

size_t log_freq = 1;
size_t exec_time = 0;
size_t step_time = 1;
size_t calc_depth = 2;

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

	sandbox.add_worksets(gravity::classical(&sandbox));
	sandbox.add_worksets(motion::classical(&sandbox, calc_depth));

	auto& xs = sandbox.database.at("x position");
	auto& ys = sandbox.database.at("y position");

	auto& v_xs = sandbox.database.at("x velocity");
	auto& v_ys = sandbox.database.at("y velocity");

	auto& ms = sandbox.database.at("mass");
	ms[0] = 2.0 * pow(10.0, 30.0);

	auto theta = random(-M_PI, M_PI);

	xs[1] = 6.0 * pow(10.0, 10.0) * cos(theta);
	ys[1] = 6.0 * pow(10.0, 10.0) * sin(theta);
	v_xs[1] = 4.7 * pow(10.0, 4.0) * -sin(theta);
	v_ys[1] = 4.7 * pow(10.0, 4.0) * cos(theta);
	ms[1] = 6.1 * pow(10.0, 10.0);

	theta = random(-M_PI, M_PI);

	xs[2] = 1.0 * pow(10.0, 11.0) * cos(theta);
	ys[2] = 1.0 * pow(10.0, 11.0) * sin(theta);
	v_xs[2] = 3.5 * pow(10.0, 4.0) * -sin(theta);
	v_ys[2] = 3.5 * pow(10.0, 4.0) * cos(theta);
	ms[2] = 4.9 * pow(10.0, 24.0);

	theta = random(-M_PI, M_PI);

	xs[3] = 1.5 * pow(10.0, 11.0) * cos(theta);
	ys[3] = 1.5 * pow(10.0, 11.0) * sin(theta);
	v_xs[3] = 3.0 * pow(10.0, 4.0) * -sin(theta);
	v_ys[3] = 3.0 * pow(10.0, 4.0) * cos(theta);
	ms[3] = 6.0 * pow(10.0, 24.0);

	theta = random(-M_PI, M_PI);

	xs[4] = 2.2 * pow(10.0, 11.0) * cos(theta);
	ys[4] = 2.2 * pow(10.0, 11.0) * sin(theta);
	v_xs[4] = 2.4 * pow(10.0, 4.0) * -sin(theta);
	v_ys[4] = 2.4 * pow(10.0, 4.0) * cos(theta);
	ms[4] = 6.4 * pow(10.0, 23.0);

	theta = random(-M_PI, M_PI);

	xs[5] = 7.8 * pow(10.0, 11.0) * cos(theta);
	ys[5] = 7.8 * pow(10.0, 11.0) * sin(theta);
	v_xs[5] = 1.3 * pow(10.0, 4.0) * -sin(theta);
	v_ys[5] = 1.3 * pow(10.0, 4.0) * cos(theta);
	ms[5] = 1.9 * pow(10.0, 27.0);

	theta = random(-M_PI, M_PI);

	xs[6] = 1.4 * pow(10.0, 12.0) * cos(theta);
	ys[6] = 1.4 * pow(10.0, 12.0) * sin(theta);
	v_xs[6] = 9.7 * pow(10.0, 3.0) * -sin(theta);
	v_ys[6] = 9.7 * pow(10.0, 3.0) * cos(theta);
	ms[6] = 5.7 * pow(10.0, 26.0);

	theta = random(-M_PI, M_PI);

	xs[7] = 2.9 * pow(10.0, 12.0) * cos(theta);
	ys[7] = 2.9 * pow(10.0, 12.0) * sin(theta);
	v_xs[7] = 6.8 * pow(10.0, 3.0) * -sin(theta);
	v_ys[7] = 6.8 * pow(10.0, 3.0) * cos(theta);
	ms[7] = 8.7 * pow(10.0, 25.0);

	theta = random(-M_PI, M_PI);

	xs[8] = 4.5 * pow(10.0, 12.0) * cos(theta);
	ys[8] = 4.5 * pow(10.0, 12.0) * sin(theta);
	v_xs[8] = 5.4 * pow(10.0, 3.0) * -sin(theta);
	v_ys[8] = 5.4 * pow(10.0, 3.0) * cos(theta);
	ms[8] = 1.0 * pow(10.0, 26.0);

	theta = random(-M_PI, M_PI);

	xs[9] = 6.3 * pow(10.0, 12.0) * cos(theta);
	ys[9] = 6.3 * pow(10.0, 12.0) * sin(theta);
	v_xs[9] = 4.7 * pow(10.0, 3.0) * -sin(theta);
	v_ys[9] = 4.7 * pow(10.0, 3.0) * cos(theta);
	ms[9] = 1.3 * pow(10.0, 22.0);

	sandbox.add_worksets(time::constant(&sandbox));
	sandbox.config.at("time change") = step_time * 1.0;

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
	buffer.validate = LSCB_VALIDATE_CHAR;

	ret = LSCb_alloc(&buffer);
	if(ret != LSCE_OK) {
		tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		puts("Error initialising libScricon.");
		printf("\033[?25h");
		exit(4);
	}

	if(strlen(output) && strcmp(output, "-")) {
		sandbox.add_worksets(logging::csv(
			&sandbox, output, log_freq, 10,
			{
				"x position", "y position",
				"x velocity", "y velocity",
				"x acceleration", "y acceleration",
			},
			{"time", "time change"}
		));
	}

	sandbox.add_worksets(
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

	puts("    -t, --step-time SECS      set the time per simulation step");
	puts("    -T, --exec-time SECS      set the execution time");
	puts("    -c, --calc-depth N        set the depth of the motion calculus\n");

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

	var = LCv_new(); var -> id = "calc_depth";
	var -> fmt = "%zu"; var -> data = &calc_depth;
	arg = LCa_new(); arg -> long_flag = "calc-depth";
	arg -> short_flag = 'c'; arg -> var = var;

	int ret = LCa_read(argc, argv);
	if(ret != LCA_OK) help(1);
}

void renderer(sandbox_t* s, void* arg) {
	static auto& tick = get<size_t>(s -> config["simulation tick"]);
	if(tick % log_freq) return;
	(void) arg;
	(void) s;

	static auto& xs = sandbox.database.at("x position");
	static auto& ys = sandbox.database.at("y position");

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

	static auto& v_xs = sandbox.database.at("x velocity");
	static auto& v_ys = sandbox.database.at("y velocity");

	auto max_vx = 0.0, max_vy = 0.0;

	for(const auto& i: v_xs) {
		const auto v_x = abs(get<double>(i));
		if(v_x > max_vx) max_vx = v_x;
	}

	for(const auto& i: v_ys) {
		const auto v_y = abs(get<double>(i));
		if(v_y > max_vy) max_vy = v_y;
	}

	LSCb_clear(&buffer);

	auto delta_x = max_x - min_x;
	auto delta_y = max_y - min_y;

	if(delta_x == 0.0) delta_x = 1.0;
	if(delta_y == 0.0) delta_y = 1.0;

	const auto delta = delta_x > delta_y? delta_x: delta_y;

	auto ix = xs.begin();
	auto ivx = v_xs.begin();
	auto ivy = v_ys.begin();
	auto ind = '0';

	for(const auto& iy: ys) {
		const auto x = get<double>(*ix);
		const auto y = get<double>(iy);
		const auto v_x = get<double>(*ivx);
		const auto v_y = get<double>(*ivy);

		const auto x_start = LSCb_getx(&buffer,
				2.0 * (x - min_x) / delta - delta_x / delta
				- v_x / max_vx
			);

		const auto y_start = LSCb_gety(&buffer,
				2.0 * (y - min_y) / delta - delta_y / delta
				- v_y / max_vy
			);

		const auto x_end = LSCb_getx(&buffer,
				2.0 * (x - min_x) / delta - delta_x / delta
			);

		const auto y_end = LSCb_gety(&buffer,
				2.0 * (y - min_y) / delta - delta_y / delta
			);
		
		LSCl_drawz(&buffer, x_start, y_start, -0.1, x_end, y_end, -0.1);
		LSCb_setzv(&buffer, x_end, y_end, 0.0, ind++);

		advance(ix, 1);
		advance(ivx, 1);
		advance(ivy, 1);
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