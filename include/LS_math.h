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

#include <threads.h>

#ifndef LS_MATH_H
#define LS_MATH_H 1

#define LSm_rdivu(a, b) ((a + (b / 2)) / b)

#define LSm_rdivd(a, b) (((a < 0) ^ (b < 0)) \
	? ((a - b / 2) / b) \
	: ((a + b / 2) / b))

#define LSM_ADD 0
#define LSM_SUB 1
#define LSM_MUL 2
#define LSM_DIV 3

typedef struct {
	int op;

	mtx_t *inp1_mtx;
	double *inp1;

	mtx_t *inp2_mtx;
	double *inp2;

	mtx_t *ret_mtx;
	double *ret;

} LSm_inp_t;

extern int LSm_do(void *input);

#endif