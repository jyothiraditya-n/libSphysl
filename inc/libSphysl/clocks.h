/* The Sphysl Project (C) 2022 Jyothiraditya Nellakra
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

#include <libSphysl.h>

#ifndef LS_CLOCKS_H
#define LS_CLOCKS_H 1
namespace libSphysl::clocks {

libSphysl::engine_t system(libSphysl::sandbox_t* s);

libSphysl::engine_t constrained(libSphysl::sandbox_t* s,
	double min, double max);

}
#endif