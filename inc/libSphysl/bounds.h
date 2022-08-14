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

#include <fstream>

#include <libSphysl.h>

#ifndef LS_BOUNDS_H
#define LS_BOUNDS_H 1
namespace libSphysl::bounds {

libSphysl::engine_t box(libSphysl::sandbox_t* s,
	double x_min, double y_min, double z_min,
	double x_max, double y_max, double z_max);

}
#endif