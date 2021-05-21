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
#include <stdio.h>
#include <LS_error.h>

#define MAX_ERR 6

bool LSe_auto = false;

thread_local int LS_errno;

static const char *errors[MAX_ERR + 1] = {
	"Unknown error: 0",
	"Memory allocation error: 1",
	"Mutex initialisation error: 2",
	"Mutex locking error: 3",
	"Mutex unlocking error: 4",
	"Thread creation error: 5",
	"Thread joining error: 6"
};

const char *LS_strerror(int err) {
	if(err < 0 || err > MAX_ERR) err = 0;
	return errors[err];
}

void LS_perror() {
	fprintf(stderr, "libSphysl: %s", LS_strerror(LS_errno));
	return;
}