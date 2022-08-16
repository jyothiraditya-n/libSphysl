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

#include <libSphysl.h>
#include <libSphysl/utility.h>

using namespace libSphysl;
using namespace libSphysl::utility;

thread_t::thread_t() {}

thread_t::thread_t(const thread_t& t):
	calculator(t.calculator), sandbox(t.sandbox), args(t.args)
{}

workset_t::workset_t(sandbox_t* s, const engine_t e) {
	threads = &(s -> threads);

	const auto total = e.args.size();
	auto concurrency = threads -> size();
	concurrency = total > concurrency? concurrency: total;

	const auto per_thread = total / concurrency;
	const auto first_thread = per_thread + (total % concurrency);

	listing_t listing = listing_t();
	listing.first = e.calculator;

	auto it = e.args.begin();
	for(size_t i = 0; i < concurrency; i++) {
		const auto limit = i? per_thread: first_thread;

		for(size_t j = 0; j < limit; j++) {
			listing.second.push_back(*it);
			std::advance(it, 1);
		}

		listings.push_back(listing);
		listing.second.clear();
	}
}

void workset_t::run() {
	const auto start = threads -> begin();
	auto it = start;

	for(auto& i: listings) {
		it -> calculator = &i.first;
		it -> args = &i.second;
		it -> start.unlock();

		std::advance(it, 1);
	}

	it = start;
	for(const auto& i: listings) {
		it -> start.lock();
		(void) i;

		std::advance(it, 1);
	}

	it = start;
	for(const auto& i: listings) {
		it -> stop.lock();
		it -> stop.unlock();
		(void) i;

		std::advance(it, 1);
	}
}

void sandbox_t::add_engine(const engine_t e) {
	engines.push_back(e);

	workset_t workset(this, e);
	worksets.push_back(workset);
}

void sandbox_t::add_engine(const std::list<engine_t> e) {
	for(const auto& i: e) {
		this -> add_engine(i);
	}
}

sandbox_t::sandbox_t() {
	threads = std::list<thread_t>(std::thread::hardware_concurrency());

	for(auto& i: threads) {
		i.sandbox = this;
	}
}

sandbox_t::~sandbox_t() {
	for(auto& i: engines) {
		i.destructor(&i);
	}
}

static void helper_kernel(thread_t* t) {
loop:	t -> start.lock();
	t -> stop.lock();
	t -> start.unlock();

	for(const auto& i: *(t -> args)) {
		(*(t -> calculator))(t -> sandbox, i);
	}

	t -> stop.unlock();

	if(t -> finished == true) return;
	else goto loop;
}

static void main_kernel(sandbox_t *s) {
	while(!s -> finished) {
		for(auto& i: s -> worksets) {
			i.run();
		}
	}
}

void sandbox_t::start() {
	for(auto& i: threads) {
		i.finished = false;
		i.start.lock();
		i.thread = std::thread{helper_kernel, &i};
	}

	finished = false;
	main_thread = std::thread{main_kernel, this};
}

void sandbox_t::stop() {
	finished = true;
	main_thread.join();

	for(auto& i: threads) {
		auto calculator = calculator_t{null_calculator};
		auto args = std::list<void*>{};

		i.finished = true;
		i.calculator = &calculator;
		i.args = &args;

		i.start.unlock();
		i.thread.join();
	}
}