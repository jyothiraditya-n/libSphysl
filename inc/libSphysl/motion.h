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

#ifndef LS_MOTION_H
#define LS_MOTION_H 1
namespace libSphysl::motion {

/* Function Declarations */

/* These are engine generators for calculating and applying linear motion to
 * entities. When the forces on the entities are continuous functions with
 * respect to time, a smoothing depth can be specified, which will be used to
 * determine how many terms of the Maclaurin series to track for each entity's
 * movement variables. */

/* Naturally, movement can be either follow classical mechanics or relativistic
 * mechanics. */

std::list<libSphysl::engine_t> classical(libSphysl::sandbox_t* s);
std::list<libSphysl::engine_t> classical(
        libSphysl::sandbox_t* s, size_t smoothing
);

std::list<libSphysl::engine_t> relativistic(libSphysl::sandbox_t* s);
std::list<libSphysl::engine_t> relativistic(libSphysl::sandbox_t* s,
        size_t smoothing
);

/* The relevant config value in the sandbox is "time change" (double). */

/* The relevant database values in the sandbox are "mass" (double)
 * "x position" (double), "y position" (double), "z position" (double),
 * "x velocity" (double), "y velocity" (double), "z velocity" (double),
 * "x accerelation" (double), "y acceleration" (double), "z acceleration",
 * (double), "x force" (double), "y force" (double), "z force" (double). */

}
#endif