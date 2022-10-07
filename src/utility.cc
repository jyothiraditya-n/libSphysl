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

libSphysl::utility::vector_t::vector_t(double x, double y, double z):
	x{x}, y{y}, z{z}
{}

libSphysl::utility::vector_t::vector_t(
	libSphysl::utility::slice_t<double> x,
	libSphysl::utility::slice_t<double> y,
	libSphysl::utility::slice_t<double> z
):
	x{x()}, y{y()}, z{z()} // Call operator() to get their values.
{}

double libSphysl::utility::vector_t::length() const{
	/* Pythagoras theorem. */
	return std::sqrt(x * x + y * y + z * z);
}

double libSphysl::utility::vector_t::length_sq() const{
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

libSphysl::utility::vector_t
libSphysl::utility::vector_t::proj(const vector_t& v) const{
	/* If this vector is the zero vector, we can't project onto it. */
	const auto length_sq = this -> length_sq();

	if(length_sq != 0.0) {
		/* proj_u(v) = v.u^ * u^; u^ is the unit vector along u.
		 * = (v.u * u) / len(u)^2 */
		return (*this) * this -> dot(v) / length_sq;
	}

	else {
		return v;
	}
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
	const size_t start, const size_t stop, const size_t divisions
){
	/* Calculations for the grouping process. */
	const auto total = stop - start;
	const auto per_group = total / divisions;
	const auto first_groups = total % divisions;
	// When the total doesn't evenly divide into the divisions, the first
	// few groups will each have one more member than the rest.

	/* The vector we are going to be returning */
	std::vector<std::pair<size_t, size_t>> ret{};

	/* Iterating through the entire range, we'll also step through the
	 * groups. The first groups will get one more element each. */
	auto beginning = start;
	for(size_t i = 0; i < divisions; i++) {
		const auto end = i < first_groups?
			beginning + per_group + 1 : beginning + per_group;

		ret.push_back({beginning, end});
		beginning = end;
	}

	/* All done. */
	return ret;
}

/* Helper function to get the groupings as a list of vectors containing the
 * indexes for the group members. */
static std::list<std::vector<size_t>> list_combinations(
	const size_t start, const size_t stop, const size_t groupings
){
	/* Return value. */
	std::list<std::vector<size_t>> ret{};

	/* Construct a vector of only one element for the range if we've been
	 * given only groupings of 1 to work with, and then return it. */
	if(groupings == 1) {
		for(size_t i = start; i < stop; i++) {
			ret.push_back({i});
		}

		return ret;
	}

	/* Iterate through all of the possible values for our group leader,
	 * leaving enough room from the end of the list to have at least one
	 * set of groupmates. */
	for(size_t i = start; i <= stop - groupings; i++) {
		/* Get the list of groupmates (subgroups) by calling ourselves
		 * recursively. */
		std::list<std::vector<size_t>> subgroups
			= list_combinations(i + 1, stop, groupings - 1);

		/* Add ourselves to each list of groupmates. */
		for(auto& j: subgroups) {
			j.push_back(i);
		}

		/* Insert these groups into the growing list of our groups.  */
		ret.insert(ret.begin(), subgroups.begin(), subgroups.end());
	}

	/* Return it all. */
	return ret;
}

void libSphysl::utility::get_combinations(
	const size_t total, const size_t groupings,
	on_combination_t on_combination
){
	/* Get the list of combinations, with the range specified as starting
	 * at 0 and going up to total. */
	const auto combinations = list_combinations(0, total, groupings);

	/* Call the callback function for each group. */
	for(const auto& i: combinations) {
		on_combination(i);
	}
}

void libSphysl::utility::get_combinations_exclusive(
	const size_t total, const size_t groupings,
	on_combination_t on_combination,
	on_exclusivity_end_t on_exclusivity_end
){
	/* Get the list of combinations, with the range specified as starting
	 * at 0 and going up to total. */
	auto combinations = list_combinations(0, total, groupings);
	
	/* This vector stores whether we haved used a particular index so
	 * far while generating an exclusive list. */
	std::vector<bool> used(total, false);

	/* Keep this loop going until we've hit every single combination. */
	while(combinations.size()) {
		/* We'll keep track of combinations we weren't able to get. */
		std::list<std::vector<size_t>> leftovers{};

		/* For every combination, check if any of the indexes it uses
		 * have been used already. If so, put it in leftovers and move
		 * on, since we must preserve exclusivity. */
		for(const auto& i: combinations) {
			for(const auto& j: i) {
				if(used[j]) {
					leftovers.push_back(i);
					goto next;
				}
			}

			/* If none of the indexes have been used, this group is
			 * alright to use, so we go ahead and call the function
			 * callback. */
			on_combination(i);

			/* And after doing that, we mark the appropriate
			 * indexes as having been used. */
			for(const auto& j: i) {
				used[j] = true;
			}

		next:	continue;
		}

		/* Once we've gone through the whole combinations, the groups
		 * that are left are not made of unique members, therefore we
		 * call the calback to indicate that we're out of exclusive
		 * pairs and must begin a new group. */
		on_exclusivity_end();

		/* Move the leftovers into the current set of combinations and
		 * clear the flags for which ones are used. */
		combinations = leftovers;
		used = std::vector<bool>(total, false);
	}
}