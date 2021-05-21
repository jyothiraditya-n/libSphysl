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
#include <LS_error.h>
#include <LS_math.h>

#define LSM_MIN_OP 0
#define LSM_MAX_OP 3

static int copy(double *source, double *dest, mtx_t *mutex) {
	if(!mutex) goto next;

	int ret = mtx_lock(mutex);
	
	if(ret == thrd_error) {
		if(LSe_auto) perror("stdlib");
		LS_errno = LS_MTX_LOCK_ERR;
		return LSE_NOOP;
	}

next:
	*dest = *source;
	if(!mutex) goto end;

	ret = mtx_unlock(mutex);
	
	if(ret == thrd_error) {
		if(LSe_auto) perror("stdlib");
		LS_errno = LS_MTX_UNLOCK_ERR;
		return LSE_NO_REC;
	}

end:
	return LSE_OK;
}

int LSm_do(void *input) {
	LSm_inp_t *data = (LSm_inp_t *) input;
	if(!data -> inp1 || !data -> inp2 || !data -> ret) return LSE_ILLEGAL;

	int op = data -> op;
	if(op < LSM_MIN_OP || op > LSM_MAX_OP) return LSE_ILLEGAL;

	double inp1, inp2, result;

	int ret = copy(data -> inp1, &inp1, data -> inp1_mtx);
	if(ret != LSE_OK) return ret;

	ret = copy(data -> inp2, &inp2, data -> inp2_mtx);
	if(ret != LSE_OK) return ret;

	if(op == LSM_ADD) result = inp1 + inp2;
	else if(op == LSM_SUB) result = inp1 - inp2;
	else if(op == LSM_MUL) result = inp1 * inp2;
	else if(op == LSM_DIV) result = inp1 / inp2;

	ret = copy(&result, data -> ret, data -> ret_mtx);
	if(ret != LSE_OK) return ret;

	return LSE_OK;
}