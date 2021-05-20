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
#include <stdbool.h>
#include <stdio.h>

#ifndef LS_ERROR
#define LS_ERROR 1

#define LSE_ILLEGAL -1
#define LSE_OK 0
#define LSE_NOOP 1
#define LSE_NO_REC 2
#define LSE_YES_REC 3

#define LS_MALLOC_ERR 1

extern thread_local int LS_errno;
extern const char *LS_strerror(int err);
extern void LS_perror();

#endif