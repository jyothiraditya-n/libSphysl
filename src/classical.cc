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

#include <libSphysl/classical.h>
#include <libSphysl/util.h>

using namespace libSphysl::classical;
using namespace libSphysl::util;
using namespace libSphysl;

struct arg_t {
	double &delta_t;

	double &x, &y, &z;
	double &v_x, &v_y, &v_z;
	double &a_x, &a_y, &a_z;

	double &F_x, &F_y, &F_z;
	double &m;
};

static void calculator(sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);
	(void) s;

	data.a_x = data.F_x / data.m;
	data.a_y = data.F_y / data.m;
	data.a_z = data.F_z / data.m;

	data.v_x += data.a_x * data.delta_t;
	data.v_y += data.a_y * data.delta_t;
	data.v_z += data.a_z * data.delta_t;

	data.x += data.v_x * data.delta_t;
	data.y += data.v_y * data.delta_t;
	data.z += data.v_z * data.delta_t;

	data.F_x = data.F_y = data.F_z = 0.0;
}

engine_t motion(sandbox_t *s) {
	engine_t engine;

	engine.calculator = calculator;
	engine.destructor = destructor<arg_t>;
	engine.sandbox = s;

	auto total = std::get<std::size_t>(s -> config.at("entity count"));
	auto& delta_t = s -> config["time change"];

	std::vector<data_t> empty(total, 0.0);
	delta_t = 1.0 / 1000000.0;

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

	auto& a_xs = s -> database["x acceleration"];
	auto& a_ys = s -> database["y acceleration"];
	auto& a_zs = s -> database["z acceleration"];

	if(a_xs.size() != total) a_xs = empty;
	if(a_ys.size() != total) a_ys = empty;
	if(a_zs.size() != total) a_zs = empty;

	auto& F_xs = s -> database["x force"];
	auto& F_ys = s -> database["y force"];
	auto& F_zs = s -> database["z force"];

	if(F_xs.size() != total) F_xs = empty;
	if(F_ys.size() != total) F_ys = empty;
	if(F_zs.size() != total) F_zs = empty;

	auto& ms = s -> database["mass"];

	if(ms.size() != total) ms = std::vector<data_t>(total, 1.0);

	for(size_t i = 0; i < total; i++) {
		auto arg = new arg_t{
			std::get<double>(delta_t),

			std::get<double>(xs[i]),
			std::get<double>(ys[i]),
			std::get<double>(zs[i]),

			std::get<double>(v_xs[i]),
			std::get<double>(v_ys[i]),
			std::get<double>(v_zs[i]),

			std::get<double>(a_xs[i]),
			std::get<double>(a_ys[i]),
			std::get<double>(a_zs[i]),

			std::get<double>(F_xs[i]),
			std::get<double>(F_ys[i]),
			std::get<double>(F_zs[i]),

			std::get<double>(ms[i])
		};

		engine.args.push_back(reinterpret_cast<void*>(arg));
	}

	return engine;
}