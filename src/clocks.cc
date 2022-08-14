/* The Sphysl Project (C) 2022 Jyothiraditya Nellakra
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

#include <libSphysl/clocks.h>
#include <libSphysl/util.h>

using namespace std::chrono;

using namespace libSphysl::clocks;
using namespace libSphysl::util;
using namespace libSphysl;

struct arg_t {
	high_resolution_clock::time_point last;
	bool initialised;

	double &delta_t;
	double min, max;
};

template<bool constrained>
static void calculator(sandbox_t* s, void* arg) {
	auto &data = *reinterpret_cast<arg_t*>(arg);
	(void) s;

	if(!data.initialised) {
		data.last = high_resolution_clock::now();
		data.initialised = true;
		return;
	}

	auto now = high_resolution_clock::now();
	auto span = duration_cast<duration<double>>(now - data.last);

	if(constrained) {
		auto delta_t = span.count();

		delta_t = delta_t > data.max? data.max: delta_t;
		delta_t = delta_t < data.min? data.min: delta_t;
		
		data.delta_t = delta_t;
	}

	else {
		data.delta_t = span.count();
	}

	data.last = now;
}

engine_t system(sandbox_t* s) {
	engine_t engine;

	engine.calculator = calculator<false>;
	engine.destructor = destructor<arg_t>;
	engine.sandbox = s;

	auto& delta_t = s -> config["time change"];
	delta_t = 1.0 / 1000000.0;

	auto arg = new arg_t{{}, false, std::get<double>(delta_t), {}, {}};
	engine.args.push_back(reinterpret_cast<void*>(arg));

	return engine;
}

engine_t constrained(sandbox_t* s, double min, double max) {
	engine_t engine;

	engine.calculator = calculator<true>;
	engine.destructor = destructor<arg_t>;
	engine.sandbox = s;

	auto& delta_t = s -> config["time change"];
	delta_t = 1.0 / 1000000.0;

	auto arg = new arg_t{{}, false, std::get<double>(delta_t), min, max};
	engine.args.push_back(reinterpret_cast<void*>(arg));

	return engine;
}