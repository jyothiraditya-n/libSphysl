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

#include <libSphysl.h>

using namespace libSphysl;

thread_t::thread_t() {};
thread_t::thread_t(const thread_t &t):
	calculator(t.calculator), sandbox(t.sandbox), args(t.args) {};

static void helper_kernel(thread_t* t) {
loop:	t -> start.lock();
	t -> stop.lock();
	t -> start.unlock();

	for(auto& i: t -> args) {
		t -> calculator(t -> sandbox, i);
	}

	t -> stop.unlock();

	if(t -> finished == true) return;
	else goto loop;
}

workset_t::workset_t(sandbox_t *s, engine_t e, std::size_t concurrency) {
	auto total = e.args.size();
	concurrency = total > concurrency? concurrency: total;

	auto per_thread = total / concurrency;
	auto first_thread = per_thread + (total % concurrency);

	thread_t thread = thread_t();
	thread.calculator = e.calculator;
	thread.sandbox = s;

	auto it = e.args.begin();

	for(size_t i = 0; i < concurrency; i++) {
		auto limit = i? per_thread: first_thread;

		for(size_t j = 0; j < limit; j++) {
			thread.args.push_back(*it);
			std::advance(it, 1);
		}

		threads.push_back(thread);
		thread.args.clear();
	}
}

void workset_t::init() {
	for(auto& i: threads) {
		i.finished = false;
		i.start.lock();
		i.thread = std::thread{helper_kernel, &i};
	}
}

void workset_t::finish() {
	for(auto& i: threads) {
		i.finished = true;
		i.thread.join();
	}
}

void workset_t::run() {
	for(auto& i: threads) {
		i.start.unlock();
	}

	for(auto& i: threads) {
		i.stop.unlock();
	}
}

void sandbox_t::add_engine(engine_t e) {
	engines.push_back(e);

	workset_t workset(this, e, std::thread::hardware_concurrency());
	worksets.push_back(workset);
}

sandbox_t::~sandbox_t() {
	for(auto& i: engines) {
		i.destructor(&i);
	}
}

static void main_kernel(sandbox_t *s) {
	while(!s -> finished) {
		for(auto &i: s -> worksets) {
			i.run();
		}
	}
}

void sandbox_t::start() {
	for(auto &i: worksets) {
		i.init();
	}

	finished = false;
	thread = std::thread{main_kernel, this};
}

void sandbox_t::stop() {
	finished = true;
	thread.join();

	for(auto &i: worksets) {
		i.finish();
	}
}