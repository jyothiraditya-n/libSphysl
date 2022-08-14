/* The Sphysl Project (C) 2022 Jyothiraditya Nellakra
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

#include <libSphysl/loggers.h>
#include <libSphysl/util.h>

using namespace libSphysl::loggers;
using namespace libSphysl::util;
using namespace libSphysl;

struct arg_t {
	std::ofstream &file;
};

template <typename T, typename = std::variant_alternative_t<0, T>>
static std::ostream& operator<<(std::ostream& os, T const& v) {
	std::visit([&os](const auto& x) { os << x; }, v);
	return os;
}

static void calculator(sandbox_t* s, void* arg) {
	auto& [file] = *reinterpret_cast<arg_t*>(arg);

	for(auto &[key, value]: s -> config) {
		file << value << ", ";
	}

	for(auto &[key, array]: s -> database) {
		for(auto &i: array) {
			file << i << ", ";
		}
	}

	file << '\n';
}

engine_t csv(sandbox_t* s, std::ofstream &file) {
	engine_t engine;

	engine.calculator = calculator;
	engine.destructor = destructor<arg_t>;
	engine.sandbox = s;

	auto arg = new arg_t{file};
	engine.args.push_back(reinterpret_cast<void*>(arg));

	for(auto &[key, value]: s -> config) {
		file << key << ", ";
	}

	for(auto &[key, array]: s -> database) {
		auto limit = array.size();

		for(std::size_t i = 0; i < limit; i++) {
			file << key << "[" << i << "], ";
		}
	}

	file << '\n';
	return engine;
}