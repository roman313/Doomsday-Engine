/**
 * @file maplumpinfo.h @ingroup wadmapconverter
 *
 * @authors Copyright &copy; 2007-2012 Daniel Swanson <danij@dengine.net>
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

#ifndef __WADMAPCONVERTER_MAPLUMPINFO_H__
#define __WADMAPCONVERTER_MAPLUMPINFO_H__

#include "doomsday.h"
#include "dd_types.h"

typedef enum maplumptype_e {
    ML_INVALID          = -1,
    FIRST_MAPLUMP_TYPE  = 0,
    ML_THINGS = FIRST_MAPLUMP_TYPE, ///< Monsters, items..
    ML_LINEDEFS,                ///< LineDefs, from editing
    ML_SIDEDEFS,                ///< SideDefs, from editing
    ML_VERTEXES,                ///< Vertices, edited and BSP splits generated
    ML_SEGS,                    ///< LineSegs, from LineDefs split by BSP
    ML_SSECTORS,                ///< Subsectors, list of LineSegs
    ML_NODES,                   ///< BSP nodes
    ML_SECTORS,                 ///< Sectors, from editing
    ML_REJECT,                  ///< LUT, sector-sector visibility
    ML_BLOCKMAP,                ///< LUT, motion clipping, walls/grid element
    ML_BEHAVIOR,                ///< ACS Scripts (compiled).
    ML_SCRIPTS,                 ///< ACS Scripts (source).
    ML_LIGHTS,                  ///< Surface color tints.
    ML_MACROS,                  ///< DOOM64 format, macro scripts.
    ML_LEAFS,                   ///< DOOM64 format, segs (close subsectors).
    ML_GLVERT,                  ///< GL vertexes
    ML_GLSEGS,                  ///< GL segs
    ML_GLSSECT,                 ///< GL subsectors
    ML_GLNODES,                 ///< GL nodes
    ML_GLPVS,                   ///< GL PVS dataset
    NUM_MAPLUMP_TYPES
} MapLumpType;

/// POD structure for defining extended metadata for map data lumps.
typedef struct maplumpinfo_s {
    lumpnum_t lump; ///< Absolute lump number for the associated data.
    MapLumpType type; ///< Recognised lump data type.
    size_t length; ///< Length of the lump data in bytes.

    struct maplumpinfo_s* Init(lumpnum_t lumpNum, MapLumpType lumpType, size_t lumpLength)
    {
        lump = lumpNum;
        type = lumpType;
        length = lumpLength;
        return this;
    }
} MapLumpInfo;

#endif /* __WADMAPCONVERTER_MAPLUMPINFO_H__ */