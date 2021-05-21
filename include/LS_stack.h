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

#include <stddef.h>

#ifndef LS_STACK_H
#define LS_STACK_H 1

typedef struct LSs_frame_s {
	struct LSs_frame_s *below;
	void *data;

} LSs_frame_t;

typedef struct {
	LSs_frame_t *top;
	size_t size;

} LSs_t;

extern void LSs_init(LSs_t *stack);
extern void LSs_clear(LSs_t *stack);

extern int LSs_push(LSs_t *stack, void *data);
extern void *LSs_pop(LSs_t *stack);

#endif