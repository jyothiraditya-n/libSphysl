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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <LS_error.h>
#include <LS_stack.h>

extern void LSs_init(LSs_t *stack) {
	stack -> top = NULL;
	stack -> size = 0;
	return;
}

extern void LSs_clear(LSs_t *stack) {
	LSs_frame_t *frame = stack -> top;
	LSs_frame_t *below;

	while(frame) {
		below = frame -> below;
		free(frame);
		frame = below;
	}

	stack -> top = NULL;
	stack -> size = 0;
	return;
}

extern int LSs_push(LSs_t *stack, void *data) {
	errno = 0;
	LSs_frame_t *frame = malloc(sizeof(LSs_frame_t));

	if(!frame) {
		if(LSe_auto) perror("stdlib");
		LS_errno = LS_MALLOC_ERR;
		return LSE_NOOP;
	}

	frame -> below = stack -> top;
	frame -> data = data;

	stack -> top = frame;
	stack -> size++;

	return LSE_OK;
}

extern void *LSs_pop(LSs_t *stack) {
	LSs_frame_t *frame = stack -> top;
	if(!frame) return NULL;

	void *data = frame -> data;

	stack -> top = frame -> below;
	free(frame);

	stack -> size--;
	return data;
}