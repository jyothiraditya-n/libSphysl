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

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef LS_RAND_H
#define LS_RAND_H 1

#define LSr_8u() (uint8_t) (rand() % (UINT8_MAX + 1))
#define LSr_8d() (int8_t) (INT8_MIN + LSr_8u());

#if UINT_MAX == 0xFFFF
#define LSr_16u() ((uint16_t) rand() + (uint16_t) rand())
#else
#define LSr_16u() (uint16_t) (rand() % (UINT16_MAX + 1))
#endif
#define LSr_16d() (int16_t) (INT16_MIN + LSr_16u());


#if UINTPTR_MAX >= 0xFFFFFFFF
#define LSr_32u() ((uint32_t) rand() + (uint32_t) rand())
#define LSr_32d() (int32_t) (INT32_MIN + LSr_32u());
#endif

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
#define LSr_64u() ((uint64_t) LSr_32u() + ((uint64_t) LSr_32u() << 32))
#define LSr_64d() (int64_t) (INT64_MIN + LSr_64u());
#endif

#endif