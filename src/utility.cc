/* The Sphysl Project (C) Copyright 2022 Jyothiraditya Nellakra
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

#include <libSphysl/utility.h>

using namespace libSphysl::utility;
using namespace libSphysl;

void libSphysl::utility::null_calculator(libSphysl::sandbox_t* s, void* arg) {
	(void) s; (void) arg;
}

void libSphysl::utility::null_destructor(engine_t* e) {
	(void) e;
}

void libSphysl::utility::randomise(
	std::vector<data_t> &v, double min, double max
){
	std::random_device device;
	std::mt19937 mersenne_engine{device()};
	std::uniform_real_distribution<double> distribution(min, max);

	for(auto &i: v) {
		i = distribution(mersenne_engine);
	}
}