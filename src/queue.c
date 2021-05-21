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

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <LS_error.h>
#include <LS_math.h>
#include <LS_queue.h>
#include <LS_stack.h>

int LSq_init(LSq_t *queue) {
	LSs_init(&queue -> funcs);
	LSs_init(&queue -> inputs);
	queue -> size = 0;

	queue -> threads = 1;
	queue -> on_error = NULL;
	queue -> workers = NULL;

	errno = 0;
	int ret = mtx_init(&queue -> mutex, mtx_plain);

	if(ret == thrd_error) {
		if(LSe_auto) perror("stdlib");
		LS_errno = LS_MTX_INIT_ERR;
		return LSE_NOOP;
	}

	queue -> running = false;
	queue -> exit = false;
	return LSE_OK;
}

int LSq_clear(LSq_t *queue) {
	if(queue -> running) return LSE_ILLEGAL;
	_LSq_dealloc(queue);

	queue -> size = 0;
	queue -> threads = 1;
	queue -> workers = NULL;

	return LSE_OK;
}

int LSq_destroy(LSq_t *queue) {
	int ret = LSq_clear(queue);
	if(ret != LSE_OK) return ret;

	mtx_destroy(&queue -> mutex);
	return LSE_OK;
}

int LS_enqueue(LSq_t *queue, int (*func)(void *), void *input) {
	if(queue -> running || queue -> workers) return LSE_ILLEGAL;

	int ret = LSs_push(&queue -> funcs, (void *) func);
	if(ret == LSE_NOOP) return LSE_NOOP;

	ret = LSs_push(&queue -> inputs, input);

	if(ret == LSE_NOOP) {
		LSs_pop(&queue -> funcs);
		return LSE_NOOP;
	}

	queue -> size++;
	return LSE_OK;
}

static int populate(LSq_t *queue, size_t i, size_t size) {
	LSq_worker_t *worker = &queue -> workers[i];

	worker -> ip = 0;
	worker -> size = size;
	worker -> funcs = malloc(sizeof(int (*)(void *)) * size);

	if(!worker -> funcs) {
		perror("stdlib");
		LS_errno = LS_MALLOC_ERR;
		return LSE_REC;
	}

	worker -> inputs = malloc(sizeof(void *) * size);

	if(!worker -> inputs) {
		perror("stdlib");
		LS_errno = LS_MALLOC_ERR;
		return LSE_REC;
	}

	for(size_t i = 0; i < size; i++) {
		worker -> funcs[i] = LSs_pop(&queue -> funcs);
		worker -> inputs[i] = LSs_pop(&queue -> inputs);
	}

	return LSE_OK;
}

int LSq_ready(LSq_t *queue) {
	if(queue -> running || queue -> workers) return LSE_ILLEGAL;
	if(!queue -> threads) return LSE_ILLEGAL;

	size_t size = queue -> size;
	size_t threads = queue -> threads;
	size_t batch = LSm_rdivu(size, threads);

	queue -> workers = malloc(sizeof(LSq_worker_t) * threads);

	for(size_t i = 0; i < threads - 1; i++) {
		int ret = populate(queue, i, batch);
		if(ret != LSE_OK) return ret;

		size -= batch;
	}

	int ret = populate(queue, threads - 1, size);
	if(ret != LSE_OK) return ret;

	return LSE_OK;
}

int LSq_start(LSq_t *queue) {
	(void) queue;
	return LSE_NOOP;
}

int LSq_stop(LSq_t *queue) {
	(void) queue;
	return LSE_NOOP;
}

int LSq_pause(LSq_t *queue) {
	(void) queue;
	return LSE_NOOP;
}

int LSq_resume(LSq_t *queue) {
	(void) queue;
	return LSE_NOOP;
}

void _LSq_dealloc(LSq_t *queue) {
	if(!queue -> workers) goto end;

	size_t threads = queue -> threads;

	for(size_t i = 0; i < threads; i++) {
		LSq_worker_t *worker = &queue -> workers[i];
		if(worker -> funcs) free(worker -> funcs);
		if(worker -> inputs) free(worker -> inputs);
	}

	free(queue -> workers);

end:
	LSs_clear(&queue -> funcs);
	LSs_clear(&queue -> inputs);
	return;
}