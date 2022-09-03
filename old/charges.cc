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

#include <libSphysl/charges.h>
#include <libSphysl/utility.h>

using namespace libSphysl::charges;
using namespace libSphysl::utility;
using namespace libSphysl;

struct arg_electricity_t {
	const double &x1, &y1, &z1;
	const double &x2, &y2, &z2;

	double &F1_x, &F1_y, &F1_z;
	double &F2_x, &F2_y, &F2_z;

	const double &q1, &q2;
	const double &epsilon;
};

static void calculator_electricity(const sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_electricity_t*>(arg);
	(void) s;

	const auto r = vector_t{data.x2, data.y2, data.z2}
		- vector_t{data.x1, data.y1, data.z1};

	const auto F = (r * data.q1 * data.q2)
		/ (4.0 * M_PI * data.epsilon * std::pow(r.length(), 3.0));

	data.F1_x -= F.x; data.F1_y -= F.y; data.F1_z -= F.z;
	data.F2_x += F.x; data.F2_y += F.y; data.F2_z += F.z;
}

std::list<engine_t> libSphysl::charges::electricity(sandbox_t* s) {
	std::list<engine_t> engines;
	engine_t engine;

	engine.calculator = calculator_electricity;
	engine.destructor = destructor<arg_electricity_t>;
	engine.sandbox = s;

	const auto total = std::get<std::size_t>(
		s -> config.at("entity count")
	);

	auto& epsilon = s -> config["vacuum permittivity"]
		= 8.8541878128 * std::pow(10.0, -12.0);

	std::vector<data_t> zeros(total, 0.0);

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

	auto& qs = s -> database["charge"];
	if(qs.size() != total) qs = zeros;

	auto new_arg = [&](std::size_t j, std::size_t i) {
		return new arg_electricity_t{
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

			std::get<double>(qs[j]),
			std::get<double>(qs[j + i]),
			std::get<double>(epsilon)
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

struct arg_magnetism_t {
	const double &x1, &y1, &z1;
	const double &x2, &y2, &z2;

	const double &v1_x, &v1_y, &v1_z;
	const double &v2_x, &v2_y, &v2_z;

	double &F1_x, &F1_y, &F1_z;

	const double &q1, &q2;
	const double &mu;
};

static void calculator_magnetism(const sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_magnetism_t*>(arg);
	(void) s;

	const auto r = vector_t{data.x1, data.y1, data.z1}
		- vector_t{data.x2, data.y2, data.z2};

	const auto B = vector_t{data.v2_x, data.v2_y, data.v2_z}.cross(r)
		* data.mu * data.q2 / (4.0 * M_PI * std::pow(r.length(), 3.0));

	const auto F = vector_t{data.v1_x, data.v1_y, data.v1_z}.cross(B)
		* data.q1;

	data.F1_x -= F.x; data.F1_y -= F.y; data.F1_z -= F.z;
}

std::list<engine_t> libSphysl::charges::magnetism(sandbox_t* s) {
	std::list<engine_t> engines;
	engine_t engine;

	engine.calculator = calculator_magnetism;
	engine.destructor = destructor<arg_magnetism_t>;
	engine.sandbox = s;

	const auto total = std::get<std::size_t>(
		s -> config.at("entity count")
	);

	auto& mu = s -> config["vacuum permeability"]
		= 1.25663706212 * std::pow(10.0, -6.0);

	std::vector<data_t> zeros(total, 0.0);

	auto& xs = s -> database["x position"];
	auto& ys = s -> database["y position"];
	auto& zs = s -> database["z position"];

	if(xs.size() != total) xs = zeros;
	if(ys.size() != total) ys = zeros;
	if(zs.size() != total) zs = zeros;

	auto& v_xs = s -> database["x velocity"];
	auto& v_ys = s -> database["y velocity"];
	auto& v_zs = s -> database["z velocity"];

	if(v_xs.size() != total) v_xs = zeros;
	if(v_ys.size() != total) v_ys = zeros;
	if(v_zs.size() != total) v_zs = zeros;

	auto& F_xs = s -> database["x force"];
	auto& F_ys = s -> database["y force"];
	auto& F_zs = s -> database["z force"];

	if(F_xs.size() != total) F_xs = zeros;
	if(F_ys.size() != total) F_ys = zeros;
	if(F_zs.size() != total) F_zs = zeros;

	auto& qs = s -> database["charge"];
	if(qs.size() != total) qs = zeros;

	auto new_arg = [&](std::size_t i, std::size_t j) {
		return new arg_magnetism_t{
			std::get<double>(xs[j]),
			std::get<double>(ys[j]),
			std::get<double>(zs[j]),

			std::get<double>(xs[i]),
			std::get<double>(ys[i]),
			std::get<double>(zs[i]),

			std::get<double>(v_xs[j]),
			std::get<double>(v_ys[j]),
			std::get<double>(v_zs[j]),

			std::get<double>(v_xs[i]),
			std::get<double>(v_ys[i]),
			std::get<double>(v_zs[i]),

			std::get<double>(F_xs[j]),
			std::get<double>(F_ys[j]),
			std::get<double>(F_zs[j]),

			std::get<double>(qs[j]),
			std::get<double>(qs[i]),
			std::get<double>(mu)
		};
	};

	for(std::size_t i = 0; i < total; i++) {
		engine.args.clear();

		for(std::size_t j = 0; j < total; j++) {
			if(i == j) continue;

			auto arg = new_arg(i, j);
			engine.args.push_back(reinterpret_cast<void*>(arg));
		}

		if(engine.args.size()) engines.push_back(engine);
		engine.args.clear();
	}

	return engines;
}