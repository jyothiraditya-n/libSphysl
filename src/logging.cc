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

#include <iostream>
#include <fstream>

#include <libSphysl/logging.h>
#include <libSphysl/utility.h>

using namespace libSphysl::logging;
using namespace libSphysl::utility;
using namespace libSphysl;

struct arg_t {
	std::ofstream file;
	bool initialised;

	const std::list<std::string> config_keys;
	const std::list<std::string> database_keys;
	const std::size_t database_entries;

	std::size_t &tick;
	std::size_t frequency;
};

template <typename T, typename = std::variant_alternative_t<0, T>>
static std::ostream& operator<<(std::ostream& os, const T& v) {
	std::visit([&os](const auto& x) { os << x; }, v);
	return os;
}

static void calculator(sandbox_t* s, void* arg) {
	auto& data = *reinterpret_cast<arg_t*>(arg);
	if(data.tick % data.frequency) return;
	if(data.initialised) goto next;

	for(const auto& i: data.config_keys) {
		data.file << i << ", ";
	}

	for(const auto& i: data.database_keys) {
		for(std::size_t j = 0; j < data.database_entries; j++) {
			data.file << i << " [" << j << "], ";
		}
	}

	data.file << std::endl;
	data.initialised = true;

next:	for(const auto& i: data.config_keys) {
		data.file << s -> config.at(i) << ", ";
	}

	for(const auto& i: data.database_keys) {
		const auto& values = s -> database.at(i);

		for(std::size_t j = 0; j < data.database_entries; j++) {
			data.file << values[j] << ", ";
		}
	}

	data.file << std::endl;
}

engine_t libSphysl::logging::csv(
	sandbox_t* s, std::string filename, std::size_t frequency,
	std::size_t database_entries, std::list<std::string> database_keys,
	std::list<std::string> config_keys
){
	engine_t engine;

	engine.calculator = calculator;
	engine.destructor = destructor<arg_t>;
	engine.sandbox = s;

	auto& tick = s -> config["simulation tick"];
	tick = std::size_t(0);

	arg_t *arg = new arg_t{
		{}, false, config_keys, database_keys,
		database_entries, std::get<std::size_t>(tick), frequency
	};

	if(filename == "" || filename == "-") {
		arg -> file.basic_ios<char>::rdbuf(std::cout.rdbuf());
	}

	else {
		arg -> file = std::ofstream(filename, std::ofstream::out);
	}
	
	engine.args.push_back(reinterpret_cast<void*>(arg));
	return engine;
}