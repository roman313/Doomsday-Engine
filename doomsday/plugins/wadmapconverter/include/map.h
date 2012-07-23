/**
 * @file map.h @ingroup wadmapconverter
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

#ifndef __WADMAPCONVERTER_MAP_H__
#define __WADMAPCONVERTER_MAP_H__

#include "doomsday.h"
#include "dd_types.h"
#include "maplumpinfo.h"

typedef struct materialref_s {
    char            name[9];
    materialid_t    id; // Doomsday's unique identifier for this.
} materialref_t;

// Line sides.
#define RIGHT                   0
#define LEFT                    1

typedef struct mside_s {
    int16_t         offset[2];
    const materialref_t* topMaterial;
    const materialref_t* bottomMaterial;
    const materialref_t* middleMaterial;
    uint            sector;
} mside_t;

// Line flags
#define LAF_POLYOBJ             0x1 // Line is from a polyobject.

typedef struct mline_s {
    uint            v[2];
    uint            sides[2];
    int16_t         flags; // MF_* flags, read from the LINEDEFS, map data lump.

    // Analysis data:
    int16_t         aFlags;

    // DOOM format members:
    int16_t         dType;
    int16_t         dTag;

    // Hexen format members:
    byte            xType;
    byte            xArgs[5];

    // DOOM64 format members:
    byte            d64drawFlags;
    byte            d64texFlags;
    byte            d64type;
    byte            d64useType;
    int16_t         d64tag;

    int             ddFlags;
    uint            validCount; // Used for Polyobj LineDef collection.
} mline_t;

typedef struct msector_s {
    int16_t         floorHeight;
    int16_t         ceilHeight;
    int16_t         lightLevel;
    int16_t         type;
    int16_t         tag;
    const materialref_t* floorMaterial;
    const materialref_t* ceilMaterial;

    // DOOM64 format members:
    int16_t         d64flags;
    uint16_t        d64floorColor;
    uint16_t        d64ceilingColor;
    uint16_t        d64unknownColor;
    uint16_t        d64wallTopColor;
    uint16_t        d64wallBottomColor;
} msector_t;

typedef struct mthing_s {
    int16_t         origin[3];
    angle_t         angle;
    int16_t         doomEdNum;
    int32_t         flags;
    int32_t         skillModes;

    // Hexen format members:
    int16_t         xTID;
    byte            xSpecial;
    byte            xArgs[5];

    // DOOM64 format members:
    int16_t         d64TID;
} mthing_t;

// Hexen only (at present):
typedef struct mpolyobj_s {
    uint            idx; // Idx of polyobject
    uint            lineCount;
    uint*           lineIndices;
    int             tag; // Reference tag assigned in HereticEd
    int             seqType;
    int16_t         anchor[2];
} mpolyobj_t;

// DOOM64 only (at present):
typedef struct mlight_s {
    float           rgb[3];
    byte            xx[3];
} surfacetint_t;

typedef struct map_s {
    uint            numVertexes;
    uint            numSectors;
    uint            numLines;
    uint            numSides;
    uint            numPolyobjs;
    uint            numThings;
    uint            numLights;

    coord_t*        vertexes; // Array of vertex coords [v0:X, vo:Y, v1:X, v1:Y, ..]
    msector_t*      sectors;
    mline_t*        lines;
    mside_t*        sides;
    mthing_t*       things;
    mpolyobj_t**    polyobjs;
    surfacetint_t*  lights;

    size_t          numFlats;
    materialref_t** flats;
    size_t          numTextures;
    materialref_t** textures;

    mapformatid_t   format;

    byte*           rejectMatrix;
    void*           blockMap;
} map_t;

extern map_t* DENG_PLUGIN_GLOBAL(map);

int IsSupportedFormat(MapLumpInfo* lumpInfos[NUM_MAPLUMP_TYPES]);

int LoadMap(MapLumpInfo* lumpInfos[NUM_MAPLUMP_TYPES]);
void AnalyzeMap(void);
int TransferMap(void);

#endif /* __WADMAPCONVERTER_MAP_H__ */