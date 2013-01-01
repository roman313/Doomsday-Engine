/**
 * @file plane.h
 * Map Plane. @ingroup map
 *
 * @authors Copyright &copy; 2003-2012 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright &copy; 2006-2012 Daniel Swanson <danij@dengine.net>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#ifndef LIBDENG_MAP_PLANE
#define LIBDENG_MAP_PLANE

#include "resource/r_data.h"
#include "map/p_dmu.h"

typedef enum {
    PLN_FLOOR,
    PLN_CEILING,
    PLN_MID,
    NUM_PLANE_TYPES
} planetype_t;

#define PS_base                 surface.base
#define PS_tangent              surface.tangent
#define PS_bitangent            surface.bitangent
#define PS_normal               surface.normal
#define PS_material             surface.material
#define PS_offset               surface.offset
#define PS_visoffset            surface.visOffset
#define PS_rgba                 surface.rgba
#define PS_flags                surface.flags
#define PS_inflags              surface.inFlags

typedef struct plane_s {
    runtime_mapdata_header_t header;
    struct sector_s *sector; /// Owner of the plane.
    Surface surface;
    coord_t height; /// Current height.
    coord_t oldHeight[2];
    coord_t target; /// Target height.
    coord_t speed; /// Move speed.
    coord_t visHeight; /// Visible plane height (smoothed).
    coord_t visHeightDelta;
    planetype_t type; /// PLN_* type.
    int planeID;
} Plane;

#ifdef __cplusplus
extern "C" {
#endif

// Return the index of plane within a sector's planes array.
#define GET_PLANE_IDX(pln)      ( (int) ((pln) - (pln)->sector->planes[0]) )

/**
 * Get a property value, selected by DMU_* name.
 *
 * @param plane  Plane instance.
 * @param args  Property arguments.
 * @return  Always @c 0 (can be used as an iterator).
 */
int Plane_GetProperty(const Plane* plane, setargs_t* args);

/**
 * Update a property value, selected by DMU_* name.
 *
 * @param plane  Plane instance.
 * @param args  Property arguments.
 * @return  Always @c 0 (can be used as an iterator).
 */
int Plane_SetProperty(Plane* plane, const setargs_t* args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /// LIBDENG_MAP_PLANE
