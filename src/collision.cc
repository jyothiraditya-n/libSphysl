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
#include <tuple>

#include <libSphysl/collision.h>
#include <libSphysl/utility.h>

using namespace libSphysl::collision;
using namespace libSphysl::utility;
using namespace libSphysl;

struct arg_entities_t {
	double &x1, &y1, &z1;
	double &x2, &y2, &z2;

	double &v1_x, &v1_y, &v1_z;
	double &v2_x, &v2_y, &v2_z;

	double &width_1, &height_1, &depth_1;
	double &width_2, &height_2, &depth_2;

	double &m1, &m2;
};

static std::tuple<double, double, double, double> collide_entities(
	const double p1, const double p2,
	const double u1, const double u2,
	const double m1, const double m2,
	const double length_1, const double length_2
){
	const auto delta_p = p2 - p1;

	if(std::abs(delta_p) >= length_1 + length_2) {
		return {p1, p2, u1, u2};
	}

	const auto v1 = (((m1 - m2) * u1) + (2.0 * m2 * u2)) / (m1 + m2);
	const auto v2 = (((m2 - m1) * u2) + (2.0 * m1 * u1)) / (m2 + m1);

	auto pp1{p1}, pp2{p2};

	if(delta_p > 0) {
		const auto delta_pp = 0.5 * (length_1 + length_2 - delta_p);
		pp1 -= delta_pp;
		pp2 += delta_pp;
	}

	else {
		const auto delta_pp = 0.5 * (length_1 + length_2 + delta_p);
		pp1 += delta_pp;
		pp2 -= delta_pp;
	}

	return {pp1, pp2, v1, v2};
}

static void calculator_entities(const sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_entities_t*>(arg);
	(void) s;

	const auto [x1, x2, v1_x, v2_x] = collide_entities(
		data.x1, data.x2, data.v1_x, data.v2_x,
		data.m1, data.m2, data.width_1, data.width_2
	);

	data.x1 = x1; data.x2 = x2; data.v1_x = v1_x; data.v2_x = v2_x;

	const auto [y1, y2, v1_y, v2_y] = collide_entities(
		data.y1, data.y2, data.v1_y, data.v2_y,
		data.m1, data.m2, data.height_1, data.height_2
	);

	data.y1 = y1; data.y2 = y2; data.v1_y = v1_y; data.v2_y = v2_y;

	const auto [z1, z2, v1_z, v2_z] = collide_entities(
		data.z1, data.z2, data.v1_z, data.v2_z,
		data.m1, data.m2, data.depth_1, data.depth_2
	);

	data.z1 = z1; data.z2 = z2; data.v1_z = v1_z; data.v2_z = v2_z;
}

struct arg_wall_t {
	double &x, &y, &z;
	double &v_x, &v_y, &v_z;

	double &width_wall, &height_wall, &depth_wall;
	double &width_entity, &height_entity, &depth_entity;
};

static std::pair<double, double> collide_wall(
	const double p, const double u,
	const double length_wall, const double length_entity
){
	double pp{p}, v{u};

	if(p + length_entity > length_wall) {
		pp = length_wall - length_entity;
		v = u > 0.0? -u: u;
	}

	else if(p - length_entity < -length_wall) {
		pp = -length_wall + length_entity;
		v = u < 0.0? -u: u;
	}

	return {pp, v};
}

static void calculator_wall(const sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_wall_t*>(arg);
	(void) s;

	const auto [x, v_x] = collide_wall(
		data.x, data.v_x,
		data.width_wall, data.width_entity
	);

	data.x = x; data.v_x = v_x;

	const auto [y, v_y] = collide_wall(
		data.y, data.v_y,
		data.height_wall, data.height_entity
	);

	data.y = y; data.v_y = v_y;

	const auto [z, v_z] = collide_wall(
		data.z, data.v_z,
		data.depth_wall, data.depth_entity
	);

	data.z = z; data.v_z = v_z;
}

std::list<engine_t> libSphysl::collision::box(sandbox_t* s) {
	std::list<engine_t> engines;
	engine_t engine;

	engine.calculator = calculator_entities;
	engine.destructor = destructor<arg_entities_t>;
	engine.sandbox = s;

	const auto total = std::get<std::size_t>(
		s -> config.at("entity count")
	);

	auto& width = s -> config["bounding box width"] = 1.0;
	auto& height = s -> config["bounding box height"] = 1.0;
	auto& depth = s -> config["bounding box depth"] = 1.0;

	std::vector<data_t> zeros(total, 0.0);
	std::vector<data_t> ones(total, 1.0);

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

	auto& widths = s -> database["bounding box width"];
	auto& heights = s -> database["bounding box height"];
	auto& depths = s -> database["bounding box depth"];

	if(widths.size() != total) widths = zeros;
	if(heights.size() != total) heights = zeros;
	if(depths.size() != total) depths = zeros;

	auto& ms = s -> database["mass"];
	if(ms.size() != total) ms = ones;

	auto new_arg = [&](std::size_t j, std::size_t i) {
		return new arg_entities_t{
			std::get<double>(xs[j]),
			std::get<double>(ys[j]),
			std::get<double>(zs[j]),

			std::get<double>(xs[j + i]),
			std::get<double>(ys[j + i]),
			std::get<double>(zs[j + i]),

			std::get<double>(v_xs[j]),
			std::get<double>(v_ys[j]),
			std::get<double>(v_zs[j]),

			std::get<double>(v_xs[j + i]),
			std::get<double>(v_ys[j + i]),
			std::get<double>(v_zs[j + i]),

			std::get<double>(widths[j]),
			std::get<double>(heights[j]),
			std::get<double>(depths[j]),

			std::get<double>(widths[j + i]),
			std::get<double>(heights[j + i]),
			std::get<double>(depths[j + i]),

			std::get<double>(ms[j]),
			std::get<double>(ms[j + i])
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

	engine.calculator = calculator_wall;
	engine.destructor = destructor<arg_wall_t>;
	engine.args.clear();

	for(std::size_t i = 0; i < total; i++) {
		auto arg = new arg_wall_t{
			std::get<double>(xs[i]),
			std::get<double>(ys[i]),
			std::get<double>(zs[i]),

			std::get<double>(v_xs[i]),
			std::get<double>(v_ys[i]),
			std::get<double>(v_zs[i]),

			std::get<double>(width),
			std::get<double>(height),
			std::get<double>(depth),

			std::get<double>(widths[i]),
			std::get<double>(heights[i]),
			std::get<double>(depths[i])
		};

		engine.args.push_back(reinterpret_cast<void*>(arg));	
	}

	if(engine.args.size()) engines.push_back(engine);
	return engines;
}