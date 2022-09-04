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
	const size_t start_1, stop_1;
	const size_t start_2, stop_2;
	const double &G;

	const std::vector<double> &xs, &ys, &zs;
	const std::vector<double> &ms;

	std::vector<double> &F_xs, &F_ys, &F_zs;
};

template<bool overlap, bool groups> static void calculator(void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);

	auto run_calculation = [&](size_t i, size_t j) {
		const auto r = libSphysl::utility::vector_t{
				data.xs[j], data.ys[j], data.zs[j]
			} - libSphysl::utility::vector_t{
				data.xs[i], data.ys[i], data.zs[i]
			};

		const auto F = (r * data.G * data.ms[i] * data.ms[j])
			/ std::pow(r.length(), 3.0);

		data.F_xs[i] += F.x; data.F_xs[j] -= F.x;
		data.F_ys[i] += F.y; data.F_ys[j] -= F.y;
		data.F_zs[i] += F.z; data.F_zs[j] -= F.z;
	};


	if constexpr(!groups) {
		run_calculation(data.start_1, data.start_2);
		return;
	}

	for(size_t i = data.start_1; i < data.stop_1; i++) {
		if constexpr(overlap) {
			for(size_t j = data.start_1; j < data.stop_1; j++) {
				if(i == j) continue;
				else run_calculation(i, j);
			}
		}

		else {
			for(size_t j = data.start_2; j < data.stop_2; j++) {
				run_calculation(i, j);
			}
		}
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
			start_1, stop_1, start_2, stop_2,
			std::get<double>(G),

			std::get<std::vector<double>>(xs),
			std::get<std::vector<double>>(ys),
			std::get<std::vector<double>>(zs),

			std::get<std::vector<double>>(ms),

			std::get<std::vector<double>>(F_xs),
			std::get<std::vector<double>>(F_ys),
			std::get<std::vector<double>>(F_zs)
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
	else engine.calculator = calculator<true, true>;
	
	for(size_t i = 0; i < groups; i++) {
		auto arg = new_arg(starts[i], stops[i], {}, {});
		engine.args.push_back(reinterpret_cast<void*>(arg));
	}

	engines.push_back(engine);
	engine.args.clear();

next:	std::vector<bool> used(groups, false);
	std::vector<std::vector<bool>> done(groups, used);

	engine.calculator = groups / 2 < concurrency?
		calculator<false, false> : calculator<false, true>;

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