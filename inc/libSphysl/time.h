/* The Sphysl Project Copyright (C) 2022 Jyothiraditya Nellakra
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

/* Including Library Headerfiles */

#include <libSphysl.h>

/* Avoiding Header Redefinitions */

#ifndef LS_TIME_H
#define LS_TIME_H 1
namespace libSphysl::time {

/* Function Declarations */

/* These are engine generators for clocks; realtime synchronises changes in
 * time in the simulation with changes in system time, constrained does the
 * same but can be configured with min/max values for the time change, and
 * constant updates the clock with a constant time change per cycle. */

libSphysl::engine_t realtime(libSphysl::sandbox_t* s);
libSphysl::engine_t constrained(libSphysl::sandbox_t* s);
libSphysl::engine_t constant(libSphysl::sandbox_t* s);

}
#endif