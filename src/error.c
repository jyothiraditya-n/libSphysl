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

thread_local int LS_errno;

static const char *errors[2] = {
	"Unknown error: 0",
	"Memory allocation error: 1"
};

const char *LS_strerror(int err) {
	if(err < 0 || err > 1) err = 0;
	return errors[err];
}

void LS_perror() {
	fprintf(stderr, "libSphysl: %s", LS_strerror(LS_errno));
	return;
}