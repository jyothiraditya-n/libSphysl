/* The Sphysl Project (C) 2021 Jyothiraditya Nellakra
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

#include <stdbool.h>
#include <stddef.h>
#include <LS_stack.h>

#ifndef LS_QUEUE_H
#define LS_QUEUE_H 1

typedef struct {
	thrd_t thread;
	size_t ip;

	size_t size;
	int (**funcs)(void *);
	void **inputs;

} LSq_worker_t;

typedef struct {
	LSs_t funcs;
	LSs_t inputs;
	size_t size;

	size_t threads;
	void (*on_error)(int);
	LSq_worker_t *workers;

	mtx_t mutex;
	bool running;
	bool exit;

} LSq_t;

extern int LSq_init(LSq_t *queue);
extern int LSq_clear(LSq_t *queue);
extern int LSq_destroy(LSq_t *queue);

extern int LS_enqueue(LSq_t *queue, int (*func)(void *), void *input);
extern int LSq_ready(LSq_t *queue);

extern int LSq_start(LSq_t *queue);
extern int LSq_stop(LSq_t *queue);
extern int LSq_pause(LSq_t *queue);
extern int LSq_resume(LSq_t *queue);

extern void _LSq_dealloc(LSq_t *queue);

#endif