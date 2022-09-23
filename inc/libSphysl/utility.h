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
	double x, y, z; // Length of the vector along each axis.

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

/* A slice of a vector is an iterable peek at the vector from a defined start
 * to a defined end. It behaves (as transparently as possible) as a regular
 * variable with most of the standard operators implemented, with the caveat
 * that getting the value outside of an expression will require the use of
 * operator(), and that operator++ and operator-- are reserved for stepping
 * forward and backward in the vector without bounds checking. */

/* The understanding is that this will be extremely useful for engine
 * generators, where each thread will be running calculations for a small slice
 * of the overall number of entities, and a slice_t will be needed for every
 * variable the calculation depends on. */

/* Tl;dr: std::vector_t<double> values(10, 1.0);
 *        slice_t<double> value1(values, 0, 5), value2(values, 5, 10);
 *        for(auto& v1: value1) {
 *	          v1 += value2++;
 *        }
 *        value1.goto_begin();
 *        std::cout << value1(); // 2.0 */

template<typename T> struct slice_t {
	std::vector<T>& vector; // Vector we are slicing.
	T* start, stop; // Start and stop iterators.
	T* data; // Current iterator.

	slice_t() {}; // Don't do anything if we haven't been given anything.

	slice_t(const slice_t& slice):
		/* Initialise variables. */
		vector{slice.vector}, start{slice.start}, stop{slice.stop},
		data{slice.data}
	{}

	slice_t(std::vector<T>& vector):
		/* Initialise variables. */
		vector{vector},

		/* Set the start and stop iterators to the vector's. Set the
		 * current iterator to the start. */
		start{vector.begin()}, stop{vector.end()}, data{vector.begin()}
	{}

	slice_t(std::vector<T>& vector, size_t start, size_t stop):
		/* Initialise the variables, set the current iterator to the
		 * start. */
		vector{vector}, start{&vector[start]}, data{&vector[start]},
		stop{&vector[stop]}
	{}

	/* Advance the iterator or move backwards. */
	slice_t& operator++() {this -> data++; return *this;}
	slice_t& operator--() {this -> data--; return *this;}

	/* Advancing or moving back the iterator but for postfix-notation. */
	slice_t operator++(int) {
		slice_t slice(*this); // Save the current state of the object.
		this -> data++; // Move the iterator forwards.
		return slice; // Return the saved state.
	}

	slice_t operator--(int) {
		slice_t slice(*this); // Save state.
		this -> data--; // Move backwards.
		return slice; // Return saved state.
	}

	/* Set the ends of the slice */
	void set_begin(size_t ind) {this -> start = &(this -> vector[ind]);}
	void set_end  (size_t ind) {this -> stop  = &(this -> vector[ind]);}

	/* Move to the ends of the slice. */
	void goto_begin() {this -> data = this -> start;}
	void goto_end  () {this -> data = this -> stop; }

	/* Get the ends of the slice for ranged-based syntaxes. */
	T* begin() const{return this -> start;}
	T* end  () const{return this -> stop; }

	/* Get the value or a negated version of it. */
	T& operator()() const{return *(this -> T);}
	T  operator- () const{return -*this;      }

	/* Do something to the value and return the result. */
	T operator+(const T& t) const{return this -> data + t;}
	T operator-(const T& t) const{return this -> data - t;}
	T operator*(const T& t) const{return this -> data * t;}
	T operator/(const T& t) const{return this -> data * t;}

	/* The same, but in case we're dealing with another slice. */
	T operator+(const slice_t<T>& s) const{return this -> data + s();}
	T operator-(const slice_t<T>& s) const{return this -> data - s();}
	T operator*(const slice_t<T>& s) const{return this -> data * s();}
	T operator/(const slice_t<T>& s) const{return this -> data * s();}

	/* The same, but also store the result. */
	T& operator= (const T& t) {return this -> data =  t;}
	T& operator+=(const T& t) {return this -> data += t;}
	T& operator-=(const T& t) {return this -> data -= t;}
	T& operator*=(const T& t) {return this -> data *= t;}
	T& operator/=(const T& t) {return this -> data /= t;}

	/* The same, but if we're again dealing with another slice. */
	T& operator= (const slice_t<T>& s) {return this -> data =  s();}
	T& operator+=(const slice_t<T>& s) {return this -> data += s();}
	T& operator-=(const slice_t<T>& s) {return this -> data -= s();}
	T& operator*=(const slice_t<T>& s) {return this -> data *= s();}
	T& operator/=(const slice_t<T>& s) {return this -> data /= s();}

	/* Compare and return true or false. */
	bool operator< (const T& t) const{return this -> data <  t;}
	bool operator> (const T& t) const{return this -> data >  t;}
	bool operator==(const T& t) const{return this -> data == t;}
	bool operator<=(const T& t) const{return this -> data <= t;}
	bool operator>=(const T& t) const{return this -> data >= t;}

	/* The same, but for interacting with slices. */
	bool operator< (const slice_t<T>& s) const{return this -> data <  s();}
	bool operator> (const slice_t<T>& s) const{return this -> data >  s();}
	bool operator==(const slice_t<T>& s) const{return this -> data == s();}
	bool operator<=(const slice_t<T>& s) const{return this -> data <= s();}
	bool operator>=(const slice_t<T>& s) const{return this -> data >= s();}
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

/* The following functions are implemented as they are almost universally
 * needed by engine generators for distributing their computations across
 * threads. */

/* This will evenly divide a range into subranges and return their start and
 * stop values (start inclusive, stop non-inclusive) in pairs. */
std::vector<std::pair<size_t, size_t>> divide_range(
	const size_t start, const size_t stop, const size_t divisions
);

/* Type definitions for callback functions. */
typedef std::function<void(std::vector<size_t> combination)> on_combination_t;
typedef std::function<void()> on_exclusivity_end_t;

/* This function will get all combinations of a <total> number of entities,
 * calling the callback function with a vector of their ids, when creating
 * groups of the size <groupings> */
void get_combinations(
	const size_t total, const size_t groupings,
	on_combination_t on_combination
);

/* This does the same, but it organises the combinations into sets where each
 * combination uses each entity only once. When one set is over, it signals
 * the start of the next with a call to on_exclusivity_end. */
void get_combinations_exclusive(
	const size_t total, const size_t groupings,
	on_combination_t on_combination,
	on_exclusivity_end_t on_exclusivity_end
);

}
#endif