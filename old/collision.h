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

#include <libSphysl.h>

#ifndef LS_COLLISION_H
#define LS_COLLISION_H 1
namespace libSphysl::collision {

std::list<libSphysl::engine_t> rebound_entities(libSphysl::sandbox_t* s);

libSphysl::engine_t rebound_on_walls(libSphysl::sandbox_t *s);
libSphysl::engine_t warp_at_walls(libSphysl::sandbox_t *s);

}
#endif