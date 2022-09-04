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

#include <libSphysl/gravity.h>
#include <libSphysl/utility.h>

struct arg_t {
	const double &G;

	const double *m1_start, *m1_stop, *m2_start, *m2_stop;

	const double *x1_start, *x2_start;
	const double *y1_start, *y2_start;
	const double *z1_start, *z2_start;

	double *F1_x_start, *F2_x_start;
	double *F1_y_start, *F2_y_start;
	double *F1_z_start, *F2_z_start;
};

template<bool overlap> static void calculator(void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);

	auto run_calculation = [&](
		const double m1, const double m2,
		const double x1, const double y1, const double z1,
		const double x2, const double y2, const double z2,
		double *F1_x, double *F1_y, double *F1_z,
		double *F2_x, double *F2_y, double *F2_z
	){
		const auto r = libSphysl::utility::vector_t{x2, y2, z2}
			- libSphysl::utility::vector_t{x1, y1, z1};

		const auto F = (r * data.G * m1 * m2)
			/ std::pow(r.length(), 3.0);

		*F1_x += F.x; *F1_y += F.y; *F1_z += F.z;
		*F2_x -= F.x; *F2_y -= F.y; *F2_z -= F.z;
	};

	auto x1 = *data.x1_start, y1 = *data.y1_start, z1 = *data.z1_start;

	auto F1_x = data.F1_x_start, F1_y = data.F1_y_start;
	auto F1_z = data.F1_z_start;

	for(auto m1 = data.m1_start; m1 < data.m1_stop; m1++) {
		auto x2 = *data.x2_start, y2 = *data.y2_start;
		auto z2 = *data.z2_start;

		auto F2_x = data.F2_x_start, F2_y = data.F2_y_start;
		auto F2_z = data.F2_z_start;

		for(auto m2 = data.m2_start; m2 < data.m2_stop; m2++) {
			if constexpr(overlap) if(m1 == m2) continue;

			run_calculation(
				*m1, *m2, x1, y1, z1, x2, y2, z2,
				F1_x, F1_y, F1_z, F2_x, F2_y, F2_z
			);

			x2++, y2++, z2++, F2_x++, F2_y++, F2_z++;
		}

		x1++, y1++, z1++, F1_x++, F1_y++, F1_z++;
	}
}

std::list<libSphysl::engine_t>
libSphysl::gravity::classical(libSphysl::sandbox_t* s) {
	std::list<libSphysl::engine_t> engines;
	libSphysl::engine_t engine;

	engine.destructor = libSphysl::utility::destructor<arg_t>;

	const auto concurrency = s -> threads.size();
	const auto total = std::get<size_t>(s -> config_get("entity count"));

	auto& G = s -> config_get("gravitational constant");

	auto& xs = s -> database_get("x position");
	auto& ys = s -> database_get("y position");
	auto& zs = s -> database_get("z position");

	auto& F_xs = s -> database_get("x force");
	auto& F_ys = s -> database_get("y force");
	auto& F_zs = s -> database_get("z force");

	const auto& ms = s -> database_get("mass");

	auto new_arg = [&](
		size_t start_1, size_t stop_1, size_t start_2, size_t stop_2
	){
		return new arg_t{
			std::get<double>(G),

			&std::get<std::vector<double>>(ms)[start_1],
			&std::get<std::vector<double>>(ms)[stop_1],
			&std::get<std::vector<double>>(ms)[start_2],
			&std::get<std::vector<double>>(ms)[stop_2],

			&std::get<std::vector<double>>(xs)[start_1],
			&std::get<std::vector<double>>(xs)[start_2],
			&std::get<std::vector<double>>(ys)[start_1],
			&std::get<std::vector<double>>(ys)[start_2],
			&std::get<std::vector<double>>(zs)[start_1],
			&std::get<std::vector<double>>(zs)[start_2],

			&std::get<std::vector<double>>(F_xs)[start_1],
			&std::get<std::vector<double>>(F_xs)[start_2],
			&std::get<std::vector<double>>(F_ys)[start_1],
			&std::get<std::vector<double>>(F_ys)[start_2],
			&std::get<std::vector<double>>(F_zs)[start_1],
			&std::get<std::vector<double>>(F_zs)[start_2]
		};
	};

	const auto groups = total > (concurrency * 2)?
		(concurrency * 2) : total;

	const auto per_group = total / groups;
	const auto first_groups = total % groups;

	std::vector<size_t> starts(groups);
	std::vector<size_t> stops(groups);

	size_t start = 0;
	for(size_t i = 0; i < groups; i++) {
		const auto stop = i < first_groups?
			start + per_group + 1: start + per_group;

		starts[i] = start;
		stops[i] = stop;

		start = stop;
	}

	if(groups / 2 < concurrency) goto next;
	else engine.calculator = calculator<true>;
	
	for(size_t i = 0; i < groups; i++) {
		auto arg = new_arg(starts[i], stops[i], {}, {});
		engine.args.push_back(reinterpret_cast<void*>(arg));
	}

	engines.push_back(engine);
	engine.args.clear();

next:	std::vector<bool> used(groups, false);
	std::vector<std::vector<bool>> done(groups, used);

	engine.calculator = calculator<false>;

	auto configure_engine = [&](size_t i, size_t j) {
		if(!used[i] && !used[j]) {
			used[i] = used[j] = true;
			return;
		};

		done[i][j] = done[j][i] = true;

		if(engine.args.size()) {
			engines.push_back(engine);
			engine.args.clear();
		}

		for(size_t k = 0; k < groups; k++) {
			used[k] = false;
		}

		used[i] = used[j] = true;
	};

	for(size_t skip = 1; skip <= groups / 2; skip++) {
		for(size_t offset1 = 0; offset1 <= skip; offset1++) {
			const auto offset2 = (offset1 + skip >= groups)?
				offset1 + skip - groups : offset1 + skip;

			auto begun = false;
			for(auto i = offset1, j = offset2;

				(i != offset1 && i != offset2
				&& j != offset1 && j != offset2) || !begun;

				i = (i + skip + 1 >= groups)?
					i + skip + 1 - groups : i + skip + 1,

				j = (j + skip + 1 >= groups)?
					j + skip + 1 - groups : j + skip + 1
			){
				begun = true;
				if(done[i][j]) continue;
				
				configure_engine(i, j);
				auto arg = new_arg(
					starts[i], stops[i],
					starts[j], stops[j]
				);
				
				engine.args.push_back(
					reinterpret_cast<void*>(arg)
				);
			}
		}
	}

	if(engine.args.size()) {
		engines.push_back(engine);
		engine.args.clear();
	}

	return engines;
}