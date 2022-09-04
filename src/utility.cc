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

/* Including Library Headerfiles */

#include <libSphysl/utility.h>

/* Function Definitions */

double libSphysl::utility::vector_t::length() const{
	/* Pythagoras theorem. */
	return std::sqrt(x * x + y * y + z * z);
}

double libSphysl::utility::vector_t::lengthsq() const{
	/* Pythagoras theorem. */
	return x * x + y * y + z * z;
}

libSphysl::utility::vector_t libSphysl::utility::vector_t::operator-() const{
	/* Return a vector of our values negated. */
	return {-x, -y, -z};
}

double libSphysl::utility::vector_t::dot(const vector_t& v) const{
	/* Multiply each of the components and return the sum. */
	return x * v.x + y * v.y + z * v.z;
}

libSphysl::utility::vector_t
libSphysl::utility::vector_t::cross(const vector_t& v) const{
	/* Let's say the following are your vectors a and b:
	 * a = <a1, a2, a3>, b = <b1, b2, b3>
	 *
	 * The way you get the dot product is by getting the determinant of the
	 * following matrix:
	 * | i  j  k  |
	 * | a1 a2 a3 |
	 * | b1 b2 b3 |*/

	/* The derivation of the return expression is left as an exercise to
	 * the reader. */
	return {
		(y * v.z) - (v.y * z),
		(v.x * z) - (x * v.z),
		(x * v.y) - (v.x * y)
	};
}

libSphysl::utility::vector_t
libSphysl::utility::vector_t::operator+(const vector_t& v) const{
	// Add the components and return a vector of the results.
	return {x + v.x, y + v.y, z + v.z};
}

libSphysl::utility::vector_t
libSphysl::utility::vector_t::operator-(const vector_t& v) const{
	// Subtract the components and return a vector of the results.
	return {x - v.x, y - v.y, z - v.z};
}

libSphysl::utility::vector_t
libSphysl::utility::vector_t::operator*(const double d) const{
	// Scale the components and return the results.
	return {x * d, y * d, z * d};
}

libSphysl::utility::vector_t
libSphysl::utility::vector_t::operator/(const double d) const{
	// Scale and return the results.
	return {x / d, y / d, z / d};
}

void libSphysl::utility::null_calculator(void* arg) {
	(void) arg; // Do nothing.
}

void libSphysl::utility::null_destructor(libSphysl::engine_t* e) {
	(void) e; // Do nothing.
}

double libSphysl::utility::random(const double min, const double max) {
	std::random_device device; // Random data generator.
	std::mt19937 engine{device()}; // Random number generator.
	std::uniform_real_distribution<double> distribution(min, max);
	// Distribution generator.

	return distribution(engine); // Return the random number.
}

void libSphysl::utility::randomise(
	std::vector<double>& v, const double min, const double max
){
	std::random_device device; // Random data generator.
	std::mt19937 engine{device()}; // Random number generator.
	std::uniform_real_distribution<double> distribution(min, max);
	// Distribution generator.

	/* Loop over the vector and set all the values to random ones. */
	for(auto &i: v) {
		i = distribution(engine);
	}
}

std::vector<std::pair<size_t, size_t>> libSphysl::utility::divide_range(
	size_t start, size_t stop, size_t divisions
){
	return {};
}

void libSphysl::utility::get_combinations(
	size_t total, size_t groupings, on_combination_t on_combination
){
	return;
}

void libSphysl::utility::get_combinations_exclusive(
	size_t total, size_t groupings, on_combination_t on_combination,
	on_exclusivity_end_t on_exclusivity_end
){
	return;
}