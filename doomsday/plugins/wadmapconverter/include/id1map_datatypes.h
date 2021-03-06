/**
 * @file id1map_datatypes.h @ingroup wadmapconverter
 *
 * @authors Copyright &copy; 2007-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef WADMAPCONVERTER_ID1MAP_DATATYPES_H
#define WADMAPCONVERTER_ID1MAP_DATATYPES_H

#include "doomsday.h"
#include "dd_types.h"

#include <de/StringPool>

/// Type used to identify references to materials in the material dictionary.
typedef de::StringPool::Id MaterialDictId;

/// Sizes of the map data structures in the arrived map formats (in bytes).
#define SIZEOF_64VERTEX         (4 * 2)
#define SIZEOF_VERTEX           (2 * 2)
#define SIZEOF_64THING          (2 * 7)
#define SIZEOF_XTHING           (2 * 7 + 1 * 6)
#define SIZEOF_THING            (2 * 5)
#define SIZEOF_XLINEDEF         (2 * 5 + 1 * 6)
#define SIZEOF_64LINEDEF        (2 * 6 + 1 * 4)
#define SIZEOF_LINEDEF          (2 * 7)
#define SIZEOF_64SIDEDEF        (2 * 6)
#define SIZEOF_SIDEDEF          (2 * 3 + 8 * 3)
#define SIZEOF_64SECTOR         (2 * 12)
#define SIZEOF_SECTOR           (2 * 5 + 8 * 2)
#define SIZEOF_LIGHT            (1 * 6)

typedef struct mside_s {
    int             index;
    int16_t         offset[2];
    MaterialDictId  topMaterial;
    MaterialDictId  bottomMaterial;
    MaterialDictId  middleMaterial;
    int             sector;
} mside_t;

/// Line sides.
#define RIGHT                   0
#define LEFT                    1

/**
 * @defgroup lineAnalysisFlags  Line Analysis flags
 */
///@{
#define LAF_POLYOBJ             0x1 ///< Line defines a polyobj segment.
///@}

#define PO_LINE_START           (1) ///< Polyobj line start special.
#define PO_LINE_EXPLICIT        (5)

#define SEQTYPE_NUMSEQ          (10)

typedef struct mline_s {
    int             index;
    int             v[2];
    int             sides[2];
    int16_t         flags; ///< MF_* flags.

    // Analysis data:
    int16_t         aFlags;

    // DOOM format members:
    int16_t         dType;
    int16_t         dTag;

    // Hexen format members:
    int8_t          xType;
    int8_t          xArgs[5];

    // DOOM64 format members:
    int8_t          d64drawFlags;
    int8_t          d64texFlags;
    int8_t          d64type;
    int8_t          d64useType;
    int16_t         d64tag;

    int             ddFlags;
    uint            validCount; ///< Used for polyobj line collection.
} mline_t;

typedef struct msector_s {
    int             index;
    int16_t         floorHeight;
    int16_t         ceilHeight;
    int16_t         lightLevel;
    int16_t         type;
    int16_t         tag;
    MaterialDictId  floorMaterial;
    MaterialDictId  ceilMaterial;

    // DOOM64 format members:
    int16_t         d64flags;
    uint16_t        d64floorColor;
    uint16_t        d64ceilingColor;
    uint16_t        d64unknownColor;
    uint16_t        d64wallTopColor;
    uint16_t        d64wallBottomColor;
} msector_t;

// Thing DoomEdNums for polyobj anchors/spawn spots.
#define PO_ANCHOR_DOOMEDNUM     (3000)
#define PO_SPAWN_DOOMEDNUM      (3001)
#define PO_SPAWNCRUSH_DOOMEDNUM (3002)

typedef struct mthing_s {
    int             index;
    int16_t         origin[3];
    angle_t         angle;
    int16_t         doomEdNum;
    int32_t         flags;
    int32_t         skillModes;

    // Hexen format members:
    int16_t         xTID;
    int8_t          xSpecial;
    int8_t          xArgs[5];

    // DOOM64 format members:
    int16_t         d64TID;
} mthing_t;

typedef struct mpolyobj_s {
    int             index;
    int             lineCount;
    int            *lineIndices;
    int             tag;
    int             seqType;
    int16_t         anchor[2];
} mpolyobj_t;

typedef struct mlight_s {
    int             index;
    float           rgb[3];
    int8_t          xx[3];
} surfacetint_t;

#endif /* WADMAPCONVERTER_ID1MAP_DATATYPES_H */
