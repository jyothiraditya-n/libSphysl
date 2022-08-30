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

#include <libSphysl/gravity.h>
#include <libSphysl/utility.h>

using namespace libSphysl::gravity;
using namespace libSphysl::utility;
using namespace libSphysl;

struct arg_t {
	const double &x1, &y1, &z1;
	const double &x2, &y2, &z2;

	double &F1_x, &F1_y, &F1_z;
	double &F2_x, &F2_y, &F2_z;

	const double &m1, &m2;
	const double &G;
};

static void calculator(const sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);
	(void) s;

	const auto r = vector_t{data.x2, data.y2, data.z2}
		- vector_t{data.x1, data.y1, data.z1};

	const auto F = (r * data.G * data.m1 * data.m2)
		/ std::pow(r.length(), 3.0);

	data.F1_x += F.x; data.F1_y += F.y; data.F1_z += F.z;
	data.F2_x -= F.x; data.F2_y -= F.y; data.F2_z -= F.z;
}

std::list<engine_t> libSphysl::gravity::classical(sandbox_t* s) {
	std::list<engine_t> engines;
	engine_t engine;

	engine.calculator = calculator;
	engine.destructor = destructor<arg_t>;
	engine.sandbox = s;

	const auto total = std::get<std::size_t>(
		s -> config.at("entity count")
	);

	auto& G = s -> config["gravitational constant"]
		= 6.67430 * std::pow(10.0, -11.0);

	std::vector<data_t> zeros(total, 0.0);
	std::vector<data_t> ones(total, 1.0);

	auto& xs = s -> database["x position"];
	auto& ys = s -> database["y position"];
	auto& zs = s -> database["z position"];

	if(xs.size() != total) xs = zeros;
	if(ys.size() != total) ys = zeros;
	if(zs.size() != total) zs = zeros;

	auto& F_xs = s -> database["x force"];
	auto& F_ys = s -> database["y force"];
	auto& F_zs = s -> database["z force"];

	if(F_xs.size() != total) F_xs = zeros;
	if(F_ys.size() != total) F_ys = zeros;
	if(F_zs.size() != total) F_zs = zeros;

	auto& ms = s -> database["mass"];
	if(ms.size() != total) ms = ones;

	auto new_arg = [&](std::size_t j, std::size_t i) {
		return new arg_t{
			std::get<double>(xs[j]),
			std::get<double>(ys[j]),
			std::get<double>(zs[j]),

			std::get<double>(xs[j + i]),
			std::get<double>(ys[j + i]),
			std::get<double>(zs[j + i]),

			std::get<double>(F_xs[j]),
			std::get<double>(F_ys[j]),
			std::get<double>(F_zs[j]),

			std::get<double>(F_xs[j + i]),
			std::get<double>(F_ys[j + i]),
			std::get<double>(F_zs[j + i]),

			std::get<double>(ms[j]),
			std::get<double>(ms[j + i]),
			std::get<double>(G)
		};
	};

	for(std::size_t i = 1; i < total; i++) {
		engine.args.clear();

		for(std::size_t k = 0; k < i; k++)
		for(std::size_t j = k; j + i < total; j += i + 1) {
			auto arg = new_arg(j, i);
			engine.args.push_back(reinterpret_cast<void*>(arg));
		}

		if(engine.args.size()) engines.push_back(engine);
		engine.args.clear();

		for(std::size_t j = i; j + i < total; j += i + 1) {
			auto arg = new_arg(j, i);
			engine.args.push_back(reinterpret_cast<void*>(arg));
		}

		if(engine.args.size()) engines.push_back(engine);
	}

	return engines;
}