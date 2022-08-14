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

#include <cstdint>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#ifndef LIBSPHYSL_H
#define LIBSPHYSL_H 1
namespace libSphysl {

inline const auto version = 1;
inline const auto subversion = 0;
inline const auto version_name = "Dust on the Floor";

typedef std::variant<bool, std::size_t, std::intmax_t, double, void*> data_t;
typedef std::map<std::string, std::vector<data_t>> database_t;
typedef std::map<std::string, data_t> config_t;

struct sandbox_t;
struct engine_t;

typedef std::function<void(sandbox_t* s, void* arg)> calculator_t;
typedef std::function<void(engine_t* e)> destructor_t;

struct thread_t {
	calculator_t calculator{};
	sandbox_t *sandbox = NULL;
	std::list<void*> args{};

	std::thread thread{};
	std::mutex start{}, stop{};
	bool finished = false;

	thread_t();
	thread_t(const thread_t &t);
};

struct workset_t {
	std::list<thread_t> threads{};

	workset_t(sandbox_t *s, engine_t e, std::size_t concurrency);

	void init();
	void finish();

	void run();
};

struct sandbox_t {
	std::list<workset_t> worksets{};
	database_t database{};
	config_t config{};

	std::list<engine_t> engines;
	void add_engine(engine_t e);
	~sandbox_t();

	std::thread thread{};
	bool finished = false;

	void start();
	void stop();
};

struct engine_t {
	calculator_t calculator;
	destructor_t destructor;

	sandbox_t *sandbox;
	std::list<void*> args;
};

}
#endif