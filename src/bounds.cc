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

#include <libSphysl/bounds.h>
#include <libSphysl/util.h>

using namespace libSphysl::bounds;
using namespace libSphysl::util;
using namespace libSphysl;

struct arg_t {
	double &x, &y, &z;
	double &v_x, &v_y, &v_z;
	
	double x_min, y_min, z_min;
	double x_max, y_max, z_max;
};

static void calculator(sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);
	(void) s;

	if(data.x > data.x_max) {
		data.x = data.x_max;
		data.v_x = -data.v_x;
	}

	else if(data.x < data.x_min) {
		data.x = data.x_min;
		data.v_x = -data.v_x;
	}

	if(data.y > data.y_max) {
		data.y = data.y_max;
		data.v_y = -data.v_y;
	}

	else if(data.y < data.y_min) {
		data.y = data.y_min;
		data.v_y = -data.v_y;
	}

	if(data.z > data.z_max) {
		data.z = data.z_max;
		data.v_z = -data.v_z;
	}

	else if(data.z < data.z_min) {
		data.z = data.z_min;
		data.v_z = -data.v_z;
	}
}

engine_t box(sandbox_t* s,
	double x_min, double y_min, double z_min,
	double x_max, double y_max, double z_max)
{
	engine_t engine;

	engine.calculator = calculator;
	engine.destructor = destructor<arg_t>;
	engine.sandbox = s;

	auto total = std::get<std::size_t>(s -> config.at("entity count"));
	std::vector<data_t> empty(total, 0.0);

	auto& xs = s -> database["x position"];
	auto& ys = s -> database["y position"];
	auto& zs = s -> database["z position"];

	if(xs.size() != total) xs = empty;
	if(ys.size() != total) ys = empty;
	if(zs.size() != total) zs = empty;

	auto& v_xs = s -> database["x velocity"];
	auto& v_ys = s -> database["y velocity"];
	auto& v_zs = s -> database["z velocity"];

	if(v_xs.size() != total) v_xs = empty;
	if(v_ys.size() != total) v_ys = empty;
	if(v_zs.size() != total) v_zs = empty;

	for(size_t i = 0; i < total; i++) {
		auto arg = new arg_t{
			std::get<double>(xs[i]),
			std::get<double>(ys[i]),
			std::get<double>(zs[i]),

			std::get<double>(v_xs[i]),
			std::get<double>(v_ys[i]),
			std::get<double>(v_zs[i]),

			x_min, y_min, z_min,
			x_max, y_max, z_max
		};

		engine.args.push_back(reinterpret_cast<void*>(arg));
	}

	return engine;
}