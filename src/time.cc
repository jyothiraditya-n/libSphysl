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

#include <libSphysl/time.h>
#include <libSphysl/utility.h>

using namespace std::chrono;

using namespace libSphysl::time;
using namespace libSphysl::utility;
using namespace libSphysl;

struct arg_realtime_t {
	high_resolution_clock::time_point last;
	bool initialised;

	double &t, &delta_t;
	double &min, &max;

	std::size_t &tick;
};

static void calculator_realtime(sandbox_t* s, void* arg) {
	auto &data = *reinterpret_cast<arg_realtime_t*>(arg);
	(void) s;

	if(!data.initialised) {
		data.last = high_resolution_clock::now();
		data.initialised = true;
		return;
	}

	const auto now = high_resolution_clock::now();
	const auto span = duration_cast<duration<double>>(now - data.last);
	auto delta_t = span.count();

	delta_t = delta_t > data.max? data.max: delta_t;
	delta_t = delta_t < data.min? data.min: delta_t;
	
	data.delta_t = delta_t;

	data.last = now;
	data.t += data.delta_t;
	data.tick++;
}

engine_t libSphysl::time::realtime(sandbox_t* s) {
	engine_t engine;

	engine.calculator = calculator_realtime;
	engine.destructor = destructor<arg_realtime_t>;
	engine.sandbox = s;

	auto& t = s -> config["time"];
	t = 0.0;

	auto& delta_t = s -> config["time change"];
	delta_t = std::pow(10.0, -6.0);

	auto& min = s -> config["minimum time change"];
	min = std::pow(10.0, -7.0);

	auto& max = s -> config["maximum time change"];
	max = std::pow(10.0, -5.0);

	auto& tick = s -> config["simulation tick"];
	tick = std::size_t(0);

	auto arg = new arg_realtime_t{
		{}, false,
		std::get<double>(t),
		std::get<double>(delta_t),
		std::get<double>(min),
		std::get<double>(max),
		std::get<std::size_t>(tick)
	};

	engine.args.push_back(reinterpret_cast<void*>(arg));
	return engine;
}

struct arg_constant_t {
	double &t, &delta_t;
	std::size_t &tick;
};

static void calculator_constant(sandbox_t* s, void* arg) {
	auto &data = *reinterpret_cast<arg_constant_t*>(arg);
	(void) s;

	data.t += data.delta_t;
	data.tick++;
}

engine_t libSphysl::time::constant(sandbox_t* s) {
	engine_t engine;

	engine.calculator = calculator_constant;
	engine.destructor = destructor<arg_constant_t>;
	engine.sandbox = s;

	auto& t = s -> config["time"];
	t = 0.0;

	auto& delta_t = s -> config["time change"];
	delta_t = std::pow(10.0, -6.0);

	auto& tick = s -> config["simulation tick"];
	tick = std::size_t(0);

	auto arg = new arg_constant_t{
		std::get<double>(t),
		std::get<double>(delta_t),
		std::get<std::size_t>(tick)
	};

	engine.args.push_back(reinterpret_cast<void*>(arg));
	return engine;
}