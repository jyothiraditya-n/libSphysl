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

#include <random>
#include <algorithm>
#include <iterator>

#include <libSphysl.h>

#ifndef LS_UTILITY_H
#define LS_UTILITY_H 1
namespace libSphysl::utility {

template<typename T>
void destructor(libSphysl::engine_t* e) {
	for(auto& i: e -> args) {
		delete reinterpret_cast<T*>(i);
	}
}

void null_calculator(void* arg);
void null_destructor(libSphysl::engine_t* e);

template<typename T> T random(T min, T max) {
	std::random_device device;
	std::mt19937 engine{device()};
	std::uniform_int_distribution<T> distribution(min, max);

	return distribution(engine);
}

template<typename T>
void randomise(std::vector<libSphysl::data_t> &v, T min, T max) {
	std::random_device device;
	std::mt19937 engine{device()};
	std::uniform_int_distribution<T> distribution(min, max);

	for(auto &i: v) {
		i = distribution(engine);
	}
}

double random(double min, double max);
void randomise(std::vector<libSphysl::data_t> &v, double min, double max);

struct vector_t {
	double x, y, z;

	double length() const;
	double dot(const vector_t& v) const;
	vector_t cross(const vector_t& v) const;

	vector_t operator+(const vector_t& v) const;
	vector_t operator-(const vector_t& v) const;
	vector_t operator*(const double d) const;
	vector_t operator/(const double d) const;
};

}
#endif