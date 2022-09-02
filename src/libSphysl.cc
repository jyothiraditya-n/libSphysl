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

/* Including Library Headerfiles */

#include <libSphysl.h>
#include <libSphysl/utility.h>

/* Function Definitions */

libSphysl::engine_t::~engine_t() {
	/* Call the user-defined destructor. */
	this -> destructor(this);
}

libSphysl::workset_t::workset_t(
	libSphysl::sandbox_t* s,
	const libSphysl::engine_t e
):
	/* Initialise variables. */
	threads(s -> threads)
{
	/* Calculate the number of calculations per thread. */
	const auto total = e.args.size();
	auto concurrency = this -> threads.size();

	concurrency = total > concurrency? concurrency: total;
	// Can't use more threads than there are calculations.

	const auto per_thread = total / concurrency;
	const auto first_threads = total % concurrency;
	// When the total doesn't evenly divide into the concurrency, the first
	// few threads will each have one more calculation than the rest.

	/* Create the listings. */
	this -> listings = std::vector<listing_t>(concurrency);

	libSphysl::listing_t listing;
	listing.first = e.calculator;

	/* Iterating through all of the arguments, we'll also step through the
	 * threads. The first threads will get one more element each. */
	auto it = e.args.begin();

	for(size_t i = 0; i < concurrency; i++) {
		const auto limit = i < first_threads?
			per_thread + 1 : per_thread;

		std::vector<void*> args(limit);
		for(size_t j = 0; j < limit; j++, std::advance(it, 1)) {
			args[j] = *it;
		}

		listing.second = args;
		this -> listings[i] = listing;
	}
}

void libSphysl::workset_t::run() {
	/* For a full run-down on the way the threads are coordinated, please
	 * refer to the file <docs/thread_synchronisation.md>. */

	/* For every step, we are going to simultaneously iterate through the
	 * listings as well as the threads, so we should cache this iterator
	 * for better performance. */
	const auto start = this -> threads.begin();

	/* Load the listing for each thread and signal to start execution. */
	auto it = start;
	for(auto& i: this -> listings) {
		it -> listing = &i;
		it -> start.unlock();

		std::advance(it, 1);
	}

	/* Once all the threads are running, come back and relock the start
	 * mutex so that they stop once they're done with the listing. */
	it = start;
	for(const auto& i: this -> listings) {
		(void) i;
		// This use of the iterators in these loops may seem weird but
		// is necessary as there may be fewer listings than threads.

		it -> start.lock();

		std::advance(it, 1);
	}

	/* Now we wait for the threads to finish up their work by trying to
	 * lock the stop mutex, which blocks until the thread unlocks it first.
	 * We then unlock the mutex so that the thread can reset, ready for the
	 * next workset. */
	it = start;
	for(const auto& i: this -> listings) {
		(void) i;

		it -> stop.lock();
		it -> stop.unlock();

		std::advance(it, 1);
	}
}

void libSphysl::sandbox_t::add_worksets(const engine_t e) {
	/* Save the engine so the args will be deleted when we're done. */
	this -> engines.push_back(e);

	/* Generate the workset for the engine and add it to the worksets. */
	libSphysl::workset_t workset(this, e);
	this -> worksets.push_back(workset);
}

void libSphysl::sandbox_t::add_worksets(const std::list<engine_t> e) {
	/* Add every engine individually. */
	for(const auto& i: e) {
		this -> add_worksets(i);
	}
}

libSphysl::sandbox_t::sandbox_t():
	/* Initialise variables. */
	threads(std::thread::hardware_concurrency())
{}

libSphysl::sandbox_t::sandbox_t(size_t concurrency):
	/* Initialise variables. */
	threads(concurrency)
{}

/* This is the kernel that is run by the calculation threads that's involved in
 * coordinating with the code in workset_t::run() to synchronise everything. */

static void helper_kernel(libSphysl::thread_t* t) {
	/* For a full run-down on the way the threads are coordinated, please
	 * refer to the file <docs/thread_synchronisation.md>. */

	/* Try to lock the start mutex which is unlocked as a signal to start
	 * calculations. Then unlock it and lock the stop mutex to indicate
	 * we aren't done executing code. */
loop:	t -> start.lock();
	t -> stop.lock();
	t -> start.unlock();

	/* Run the calculator on the arguments for all the calculations. */
	for(auto& i: t -> listing -> second) {
		t -> listing -> first(i);
	}

	/* Signal that we are done with code execution. */
	t -> stop.unlock();

	/* Break out if we need to stop, else go back to the top and wait to
	 * start doing calculations again. */
	if(t -> finished == true) return;
	else goto loop;
}

/* This is the kernel that's run by the main thread to keep running the various
 * worksets. The main reason it exists is so that sandbox_t::start() can return
 * to whichever function caled it. */

static void main_kernel(libSphysl::sandbox_t *s) {
	/* Keep running all the worksets until we're done. */
	while(!s -> finished) {
		for(auto& i: s -> worksets) {
			i.run();
		}
	}
}

void libSphysl::sandbox_t::start() {
	/* For a full run-down on the way the threads are coordinated, please
	 * refer to the file <docs/thread_synchronisation.md>. */

	/* Initialise the helper threads. */
	for(auto& i: threads) {
		/* Make sure the thread doesn't think we're done, and make sure
		 * it doesn't actually start doing anything until the first
		 * workset gets run. */
		i.finished = false;
		i.start.lock();

		/* Start the thread. */
		i.thread = std::thread{helper_kernel, &i};
	}

	/* Start the main thread after the helper threads are started so that
	 * the mutexes are ready to be interacted with when workset_t::run()
	 * starts manipulating them. */

	finished = false;
	// Make sure the main thread doesn't think we're done.

	main_thread = std::thread{main_kernel, this};
	// Start the main thread.
}

void libSphysl::sandbox_t::stop() {
	/* For a full run-down on the way the threads are coordinated, please
	 * refer to the file <docs/thread_synchronisation.md>. */

	/* Tell the main thread to exit before the worker threads so that it
	 * won't deadlock on trying to get them to keep running worksets. */
	finished = true;
	main_thread.join();

	/* Stop the helper threads. */
	for(auto& i: threads) {
		/* Give the helper threads a nothing burger to calculate. */
		libSphysl::listing_t listing = {
			libSphysl::utility::null_calculator,
			std::vector<void*>{}
		};

		i.listing = &listing;

		/* Tell them to finish up when they're done. */
		i.finished = true;

		/* Start them off and wait for them to finish and exit
		 * normally. */
		i.start.unlock();
		i.thread.join();
	}
}