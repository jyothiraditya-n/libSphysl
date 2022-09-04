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

/* Including Standard Libraries */

#include <random>

/* Including Library Headerfiles */

#include <libSphysl.h>

/* Avoiding Header Redefinitions */

#ifndef LS_UTILITY_H
#define LS_UTILITY_H 1
namespace libSphysl::utility {

/* Type Definitions */

/* This vector library is not intended to be used for data storage, hence why
 * it isn't part of the data_t or declared within libSphysl.h. It in mainly
 * meant to write calculations needings vector operations. */

struct vector_t {
	double x, y, z;

	/* You can either get the normal length or the square of the length,
	 * the latter being useful if you're going to run the result through
	 * std::pow() anyway since you can then avoid a needless call to
	 * sqrt(). */

	double length() const;
	double lengthsq() const;

	vector_t operator-() const; // Unary negation operator.

	double dot(const vector_t& v) const; // Gets the scalar product.
	vector_t cross(const vector_t& v) const; // Gets the vector product.

	vector_t operator+(const vector_t& v) const; // Vector operations.
	vector_t operator-(const vector_t& v) const;

	vector_t operator*(const double d) const; // Scalar operations.
	vector_t operator/(const double d) const;
};

/* Function Declarations */

/* Simple templated function to de-allocate heap-allocated arguments given
 * their type. The understanding is that this will be used by most engine
 * generators unless they're doing something specific unto themselves. */

template<typename T> void destructor(libSphysl::engine_t* e) {
	/* Loop over the arguments and delete them. */
	for(auto& i: e -> args) {
		delete reinterpret_cast<T*>(i);
	}
}

/* Commonly used functions for when you need a function of the right type that
 * does nothing. */

void null_calculator(void* arg);
void null_destructor(libSphysl::engine_t* e);

/* Get a random integer between two values, templated because it depends on
 * what kind of integer you want. */

template<typename T> T random(const T min, const T max) {
	std::random_device device; // Random data generator.
	std::mt19937 engine{device()}; // Random number generator.
	std::uniform_int_distribution<T> distribution(min, max);
	// Distribution generator.

	return distribution(engine); // Return the random number.
}

/* Overload for doubles, since getting a random double uses a different
 * distribution generator from the integers. */

double random(const double min, const double max);

/* Same as random() but fill a vector with the random values. */

template<typename T>
void randomise(std::vector<T>& v, const T min, const T max) {
	std::random_device device; // Random data generator.
	std::mt19937 engine{device()}; // Random number generator.
	std::uniform_int_distribution<T> distribution(min, max);
	// Distribution generator.

	/* Loop over the vector and set all the values to random ones. */
	for(auto &i: v) {
		i = distribution(engine);
	}
}

/* Overload for doubles for the same reason. */

void randomise(std::vector<double>& v, const double min, const double max);
}
#endif