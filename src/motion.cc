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

#include <libSphysl/motion.h>
#include <libSphysl/utility.h>

struct arg_t {
	const double &delta_t;
	const size_t start, stop;

	std::vector<double> &xs, &ys, &zs;
	std::vector<double> &v_xs, &v_ys, &v_zs;
	std::vector<double> &a_xs, &a_ys, &a_zs;

	std::vector<double> &F_xs, &F_ys, &F_zs;
	const std::vector<double> &ms;

	const size_t length, depth;
	bool initialised;

	std::vector<std::vector<std::pair<double, double>>> dv_xs, dv_ys, dv_zs;
	std::vector<std::vector<std::pair<double, double>>> da_xs, da_ys, da_zs;
	std::vector<double> coeffs;
};

static void calculator_simple(void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);

	for(size_t i = data.start; i < data.stop; i++) {
		data.a_xs[i] = data.F_xs[i] / data.ms[i];
		data.a_ys[i] = data.F_ys[i] / data.ms[i];
		data.a_zs[i] = data.F_zs[i] / data.ms[i];

		data.v_xs[i] += data.a_xs[i] * data.delta_t;
		data.v_ys[i] += data.a_ys[i] * data.delta_t;
		data.v_zs[i] += data.a_zs[i] * data.delta_t;

		data.xs[i] += data.v_xs[i] * data.delta_t;
		data.ys[i] += data.v_ys[i] * data.delta_t;
		data.zs[i] += data.v_zs[i] * data.delta_t;

		data.F_xs[i] = data.F_ys[i] = data.F_zs[i] = 0.0;
	}
}

static void calculator_predictive(void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);

	size_t e = 0, ei = data.start;
loop_1:	if(e >= data.length) goto next;

	if(!data.initialised) {
		data.dv_xs[e][0].first = data.v_xs[ei];
		data.dv_ys[e][0].first = data.v_ys[ei];
		data.dv_zs[e][0].first = data.v_zs[ei];
	}

	for(size_t j = 0; j < data.depth; j++) {
		data.da_xs[e][j].second = data.da_xs[e][j].first;
		data.da_ys[e][j].second = data.da_ys[e][j].first;
		data.da_zs[e][j].second = data.da_zs[e][j].first;

		data.dv_xs[e][j].second = data.dv_xs[e][j].first;
		data.dv_ys[e][j].second = data.dv_ys[e][j].first;
		data.dv_zs[e][j].second = data.dv_zs[e][j].first;
	}

	data.a_xs[ei] = data.da_xs[e][0].first = data.F_xs[ei] / data.ms[ei];
	data.a_ys[ei] = data.da_ys[e][0].first = data.F_ys[ei] / data.ms[ei];
	data.a_zs[ei] = data.da_zs[e][0].first = data.F_zs[ei] / data.ms[ei];

	if(!data.initialised) {
		data.da_xs[e][0].second = data.a_xs[ei];
		data.da_ys[e][0].second = data.a_ys[ei];
		data.da_zs[e][0].second = data.a_zs[ei];
	}

	e++;
	goto loop_1;

next:	data.initialised = true;
	
	e = 0; ei = data.start;
loop_2:	if(e >= data.length) return;

	data.v_xs[ei] += data.a_xs[ei] * data.delta_t;
	data.v_ys[ei] += data.a_ys[ei] * data.delta_t;
	data.v_zs[ei] += data.a_zs[ei] * data.delta_t;

	for(size_t i = 1; i < data.depth; i++) {
		data.da_xs[e][i].first = (data.da_xs[e][i - 1].first
			- data.da_xs[e][i - 1].second) / data.delta_t;

		data.da_ys[e][i].first = (data.da_ys[e][i - 1].first
			- data.da_ys[e][i - 1].second) / data.delta_t;

		data.da_zs[e][i].first = (data.da_zs[e][i - 1].first
			- data.da_zs[e][i - 1].second) / data.delta_t;

		const double integrant = std::pow(data.delta_t, i + 1.0)
			/ data.coeffs[i];

		data.v_xs[ei] += data.da_xs[e][i].first * integrant;
		data.v_ys[ei] += data.da_ys[e][i].first * integrant;
		data.v_zs[ei] += data.da_zs[e][i].first * integrant;
	}

	data.dv_xs[e][0].first = data.v_xs[ei];
	data.dv_ys[e][0].first = data.v_ys[ei];
	data.dv_zs[e][0].first = data.v_zs[ei];

	data.xs[ei] += data.v_xs[ei] * data.delta_t;
	data.ys[ei] += data.v_ys[ei] * data.delta_t;
	data.zs[ei] += data.v_zs[ei] * data.delta_t;

	for(size_t i = 1; i < data.depth; i++) {
		data.dv_xs[i][e].first = (data.dv_xs[e][i - 1].first
			- data.dv_xs[e][i - 1].second) / data.delta_t;

		data.dv_ys[i][e].first = (data.dv_ys[e][i - 1].first
			- data.dv_ys[e][i - 1].second) / data.delta_t;

		data.dv_zs[i][e].first = (data.dv_zs[e][i - 1].first
			- data.dv_zs[e][i - 1].second) / data.delta_t;

		const double integrant = std::pow(data.delta_t, i + 1.0)
			/ data.coeffs[i];

		data.xs[ei] += data.dv_xs[e][i].first * integrant;
		data.ys[ei] += data.dv_ys[e][i].first * integrant;
		data.zs[ei] += data.dv_zs[e][i].first * integrant;
	}

	data.F_xs[ei] = data.F_ys[ei] = data.F_zs[ei] = 0.0;

	e++; ei++;
	goto loop_2;
}

static size_t factorial(const size_t n) {
	auto x = 1.0;

	for(size_t i = 2; i <= n; i++) {
		x *= i;
	}

	return x;
}

template<bool predictive>
libSphysl::engine_t generator(libSphysl::sandbox_t *s, size_t depth) {
	libSphysl::engine_t engine;

	engine.calculator = predictive?
		calculator_predictive : calculator_simple;

	engine.destructor = libSphysl::utility::destructor<arg_t>;

	const auto concurrency = s -> threads.size();
	const auto total = std::get<size_t>(s -> config_get("entity count"));

	const auto threads = total > concurrency? concurrency: total;
	const auto per_thread = total / threads;
	const auto first_threads = total % threads;
	
	const auto& delta_t = s -> config_get("time change");

	auto& xs = s -> database_get("x position");
	auto& ys = s -> database_get("y position");
	auto& zs = s -> database_get("z position");

	auto& v_xs = s -> database_get("x velocity");
	auto& v_ys = s -> database_get("y velocity");
	auto& v_zs = s -> database_get("z velocity");

	auto& a_xs = s -> database_get("x acceleration");
	auto& a_ys = s -> database_get("y acceleration");
	auto& a_zs = s -> database_get("z acceleration");

	auto& F_xs = s -> database_get("x force");
	auto& F_ys = s -> database_get("y force");
	auto& F_zs = s -> database_get("z force");

	const auto& ms = s -> database_get("mass");

	auto initialiser = [&]() {
		if constexpr(!predictive) return [&](
			size_t start, size_t stop, size_t length
		){
			(void) length;

			return new arg_t{
				std::get<double>(delta_t),
				start, stop,

				std::get<std::vector<double>>(xs),
				std::get<std::vector<double>>(ys),
				std::get<std::vector<double>>(zs),

				std::get<std::vector<double>>(v_xs),
				std::get<std::vector<double>>(v_ys),
				std::get<std::vector<double>>(v_zs),

				std::get<std::vector<double>>(a_xs),
				std::get<std::vector<double>>(a_ys),
				std::get<std::vector<double>>(a_zs),

				std::get<std::vector<double>>(F_xs),
				std::get<std::vector<double>>(F_ys),
				std::get<std::vector<double>>(F_zs),

				std::get<std::vector<double>>(ms),

				{}, {}, {}, {}, {}, {}, {}, {}, {}, {}
			};
		};

		else return [&](size_t start, size_t stop, size_t length) {
			static std::vector<
				std::vector<std::pair<double, double>>
			> pairs(length, {depth, {0.0, 0.0}});

			static std::vector<double> coeffs(depth);
			if(!coeffs[0]) {
				for(size_t i = 0; i < depth; i++) {
					coeffs[i] = factorial(i + 1);
				}
			}

			return new arg_t{
				std::get<double>(delta_t),
				start, stop,

				std::get<std::vector<double>>(xs),
				std::get<std::vector<double>>(ys),
				std::get<std::vector<double>>(zs),

				std::get<std::vector<double>>(v_xs),
				std::get<std::vector<double>>(v_ys),
				std::get<std::vector<double>>(v_zs),

				std::get<std::vector<double>>(a_xs),
				std::get<std::vector<double>>(a_ys),
				std::get<std::vector<double>>(a_zs),

				std::get<std::vector<double>>(F_xs),
				std::get<std::vector<double>>(F_ys),
				std::get<std::vector<double>>(F_zs),

				std::get<std::vector<double>>(ms),

				length, depth, false,
				pairs, pairs, pairs,
				pairs, pairs, pairs,
				coeffs
			};
		};
	}();

	size_t start = 0;
	for(size_t i = 0; i < concurrency; i++) {
		const auto stop = i < first_threads?
			start + per_thread + 1 : start + per_thread;

		const auto arg = initialiser(start, stop, stop - start);

		engine.args.push_back(reinterpret_cast<void*>(arg));
		start = stop;
	}

	return engine;
}

libSphysl::engine_t libSphysl::motion::simple(libSphysl::sandbox_t *s) {
	return generator<false>(s, 0);
}

libSphysl::engine_t
libSphysl::motion::predictive(libSphysl::sandbox_t *s, size_t depth) {
	return generator<true>(s, depth);
}