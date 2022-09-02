# Thread Synchronisation

Since the only synchronisation required to prevent data races is making sure the worksets are executed sequentially, we need to make sure that we can pause the threads while we context switch from one workset to another. Functionally, this means changing the `listing_t` pointer in each thread to point to the respective thread's listing in the workset, finishing the calculations, and then moving on to the next workset in an infinite loop.

This would block the thread that calls the simulation's start function, so in addition to helper threads that run the calculations, we need a main thread that keeps doing the context switches in an infinite loop. Naturally, however, an infinite loop wouldn't quite do either, so we also need some way to terminate all the threads when we want to stop the simulation.

## How things are set up.

The main components that are involved in the thread synchronisation are thus the simulation sandbox `sandbox_t` that starts and stops everything, the thread `thread_t`s that run computations and await instruction when they're finished, and the workset `workset_t`s that load their listings into the threads when run and wait until the threads are finished. This is all done using the two mutexes `start` and `stop` as well the boolean `finished` found in each helper thread's `thread_t` structure, as well as the a boolean `finished` found in the main thread's `sandbox_t` structure.

The relevant functions are `sandbox_t::start()`, `sandbox_t::stop()` and `workset_t::run()`, which respectively start all the threads, stop them all, and context switch the listings in the helper threads, as invoked for all worksets by the main thread. The helper threads run the `helper_kernel` function as defined privately in `src/libSphysl.cc` while the main thread runs the `main_kernel` function similarly defined in `src/libSphysl.cc`.

## Starting the threads.

If `finished` is set to true, the threads would logically exit, so we need to set that to false to begin with. Then, we go ahead and launch the helper threads before the main thread. To do so, we lock the `start` mutexes and start the threads, and on their part, the helper threads block when they try to lock the `start` mutex as well. Thus, the primary tactic for communication between threads is locking a mutex and waiting for it to be unlocked.

## What the main thread does.

The function of the main thread is rather simple in that it infinitely loops through the worksets, invoking their `run()` functions, while checking that `finished` in the `sandbox_t` for the simulation hasn't been set to false. Else, it exits normally.

## The back and forth between the helper threads and `workset_t::run()`

After setting the listings that the helper threads should execute, the workset unlocks the `start` mutexes, which are then immediately locked by the helper threads that were previously blocked in their execution. The worker threads then lock the `stop` mutexes and unlock the `start` mutexes, which corresponds to the main thread which has started coming back to re-lock the `start` mutexes so that the helper threads block when they're done with their listings.

However, the main thread then blocks on trying to lock the `stop` mutexes which are only unlocked by the helper threads after they're done with their calculations. Once the main thread can lock it, it immediately unlocks it, and once it's done with this for all threads, it goes ahead and returns, having run the workset.

The helper threads, on the other hand, go ahead and check if they should exit after they unlock the `stop` mutexes and loop if execution can continue.

## Thread Termination

Within the libSphysl backend, the kernels that run on the various threads reference a boolean to check if they should terminate execution or continue going. If this variable is set, they will break out of their infinite loops, exiting normally. Namely, the main kernel refers to the `finished` variable in its respective `sandbox_t` while the helper kernels refer to `finished` in their respective `thread_t`s.

In order to avoid deadlocks, the main kernel is terminated before the helper kernels. While the main kernel is a simple enough matter of setting the flag, for each of the helper kernels, we need to also assign them a listing for a computation that involves doing nothing, and then unlock their `start` mutexes as the main kernel would have done. When they reach the end of their loop, they'll see the flag has been set and exit normally.

## Illustration of everything in action

The following is an approximate illustration of everything running on a dual-threaded workload with one workset. Naturally, exactly which instructions are being run in parallel depends on the way things were compiled, the CPU at hand, and many many other factors, but this might help shed light on the concept.

Calling Thread | Main Thread | Helper Thread #1 | Helper Thread #2
--- | --- | --- | ---
calls `start()` | | |
sets `finished` flag of thread 1 to `false` | | |
locks `start` mutex of thread 1 | | |
starts thread 1
sets `finished` flag of thread 2 to `false` | | blocks on locking `start` mutex |
locks `start` mutex of thread 2 | | |
start thread 2 | | |
sets `finished` flag of sandbox to `false` | | | blocks on locking `start` mutex
starts main thread | | |
returns to caller | checks `finished` flag; doesn't exit | |
&nbsp; | calls `run()` on workset | |
&nbsp; | sets listing of thread 1 | |
&nbsp; | unlocks `start` mutex of thread 1 | |
&nbsp; | sets listing of thread 2 | locks `start` mutex |
&nbsp; | unlocks `start` mutex of thread 2 | locks `stop` mutex |
&nbsp; | blocks on locking `start` mutex of thread 1 | unlocks `start` mutex | locks `start` mutex
&nbsp; | locks `start` mutex of thread 1 | executes listing | locks `stop` mutex
&nbsp; | blocks on locking `start` mutex of thread 2 | unlocks `stop` mutex | unlocks `start` mutex
&nbsp; | locks `start` mutex of thread 2 | checks `finished` flag; loops | executes listing
calls `stop()` | locks `stop` mutex of thread 1 | blocks on locking `start` mutex | unlocks `stop` mutex
sets `finished` flag of sandbox to `true` | locks `stop` mutex of thread 2 | | checks `finished` flag; loops
waits for main thread to join | checks `finished` flag; exits | | blocks on locking `start` mutex
sets listing for thread 1 | | |
sets `finished` flag for thread 1 to `true` | | |
unlocks `start` mutex for thread 1 | | |
waits for thread 1 to join | | locks `start` mutex |
&nbsp; | | locks `stop` mutex |
&nbsp; | | unlocks `start` mutex |
&nbsp; | | executes listing |
&nbsp; | | unlocks `stop` mutex |
&nbsp; | | checks `finished` flag; exits |
sets `finished` flag for thread 2 to `true` |
unlocks `start` mutex for thread 2 |
waits for thread 2 to join | | | locks `start` mutex
&nbsp; | | | locks `stop` mutex
&nbsp; | | | unlocks `start` mutex
&nbsp; | | | executes listing
&nbsp; | | | unlocks `stop` mutex
&nbsp; | | | checks `finished` flag; exits
returns to caller | | |