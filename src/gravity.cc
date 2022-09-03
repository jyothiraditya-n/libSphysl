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
	const size_t start, stop, skip;
	const double &G;

	const std::vector<double> &xs, &ys, &zs;
	const std::vector<double> &ms;

	std::vector<double> &F_xs, &F_ys, &F_zs;
};

static void calculator(void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);

	auto i = data.start;
	auto j = data.start + data.skip;

loop:	if(i >= data.stop) return;
	
	for(size_t k = 0; k < data.skip; k++) {
		const auto r = libSphysl::utility::vector_t{
				data.xs[j + k], data.ys[j + k], data.zs[j + k]
			} - libSphysl::utility::vector_t{
				data.xs[i + k], data.ys[i + k], data.zs[i + k]
			};

		const auto F = (r * data.G * data.ms[i + k] * data.ms[j + k])
			/ std::pow(r.length(), 3.0);

		data.F_xs[i + k] += F.x; data.F_xs[j + k] -= F.x;
		data.F_ys[i + k] += F.y; data.F_ys[j + k] -= F.y;
		data.F_zs[i + k] += F.z; data.F_zs[j + k] -= F.z;
	}

	i += data.skip + 1;
	j += data.skip + 1;

	goto loop;
}

std::list<libSphysl::engine_t> libSphysl::gravity::classical(libSphysl::sandbox_t* s) {
	std::list<libSphysl::engine_t> engines;
	libSphysl::engine_t engine;

	engine.calculator = calculator;
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

	auto initialiser = [&](size_t start, size_t stop, size_t skip) {
		return new arg_t{
			start, stop, skip,
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

	for(size_t skip = 1; skip < total - 1; skip++) {
		const auto other = total % (skip * 2);
		const auto units = total / (skip * 2)
			+ other? size_t{1}: size_t{0};

		const auto threads = units > concurrency? concurrency: units;

		const auto per_thread = units / threads;
		const auto first_threads = units % threads;

		engine.args.clear();

		size_t start = 0;
		for(size_t i = 0; i < concurrency; i++) {
			const auto stop = i < first_threads?
				start + per_thread + 1 : start + per_thread;

			const auto arg = (i + 1 < concurrency)?
				initialiser(
					start * (skip * 2),
					stop * (skip * 2),
					skip
				):
				initialiser(
					start * (skip * 2),
					total - skip,
					skip
				);

			engine.args.push_back(reinterpret_cast<void*>(arg));
			start = stop;
		}

		if(engine.args.size()) {
			engines.push_back(engine);
		}
	}

	return engines;
}