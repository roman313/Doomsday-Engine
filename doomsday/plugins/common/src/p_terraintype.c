/**\file p_terraintype.c
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2009-2011 Daniel Swanson <danij@dengine.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/**
 * Terrain Types.
 */

// HEADER FILES ------------------------------------------------------------

#include <string.h>

#if __JDOOM__
#  include "jdoom.h"
#elif __JDOOM64__
#  include "jdoom64.h"
#elif __JHERETIC__
#  include "jheretic.h"
#elif __JHEXEN__
#  include "jhexen.h"
#endif

#include "dmu_lib.h"
#include "p_terraintype.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

typedef struct {
    material_t*     material;
    uint            terrainNum;
} materialterraintype_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static terraintype_t terrainTypes[] =
{
    {"Default", 0}, // Default type (no special attributes).
#if __JDOOM__ || __JDOOM64__
    {"Water",   TTF_NONSOLID|TTF_FLOORCLIP},
    {"Lava",    TTF_NONSOLID|TTF_FLOORCLIP},
    {"Blood",   TTF_NONSOLID|TTF_FLOORCLIP},
    {"Nukage",  TTF_NONSOLID|TTF_FLOORCLIP},
    {"Slime",   TTF_NONSOLID|TTF_FLOORCLIP},
#endif
#if __JHERETIC__
    {"Water",   TTF_NONSOLID|TTF_FLOORCLIP|TTF_SPAWN_SPLASHES},
    {"Lava",    TTF_NONSOLID|TTF_FLOORCLIP|TTF_SPAWN_SMOKE},
    {"Sludge",  TTF_NONSOLID|TTF_FLOORCLIP|TTF_SPAWN_SLUDGE},
#endif
#if __JHEXEN__
    {"Water",   TTF_NONSOLID|TTF_FLOORCLIP|TTF_SPAWN_SPLASHES},
    {"Lava",    TTF_NONSOLID|TTF_FLOORCLIP|TTF_FRICTION_HIGH|TTF_DAMAGING|TTF_SPAWN_SMOKE},
    {"Sludge",  TTF_NONSOLID|TTF_FLOORCLIP|TTF_SPAWN_SLUDGE},
    {"Ice",     TTF_FRICTION_LOW},
#endif
    {NULL, 0}
};

static materialterraintype_t* materialTTypes = 0;
static uint numMaterialTTypes = 0, maxMaterialTTypes = 0;

// CODE --------------------------------------------------------------------

/**
 * @param name          The symbolic terrain type name.
 */
static uint findTerrainTypeNumForName(const char* name)
{
    if(name && name[0])
    {
        uint i;
        for(i = 0; terrainTypes[i].name; ++i)
        {
            terraintype_t* tt = &terrainTypes[i];
            if(!stricmp(tt->name, name))
                return i + 1; // 1-based index.
        }
    }
    return 0;
}

static materialterraintype_t* findMaterialTerrainType(material_t* mat)
{
    if(mat)
    {
        uint i;
        for(i = 0; i < numMaterialTTypes; ++i)
            if(materialTTypes[i].material == mat)
                return &materialTTypes[i];
    }
    return 0;
}

static __inline terraintype_t* findTerrainTypeForMaterial(material_t* mat)
{
    materialterraintype_t* mtt;
    if((mtt = findMaterialTerrainType(mat)))
        return &terrainTypes[mtt->terrainNum];
    return 0;
}

static materialterraintype_t* getMaterialTerrainType(material_t* mat, uint idx)
{
#define BATCH_SIZE       8

    materialterraintype_t* mtt;

    // If we've already assigned this material to a terrain type, override
    // the previous assignation.
    if((mtt = findMaterialTerrainType(mat)))
    {
        mtt->terrainNum = idx;
        return mtt;
    }

    // Its a new material.
    // Only allocate memory when it's needed.
    if(++numMaterialTTypes > maxMaterialTTypes)
    {
        uint newMax = maxMaterialTTypes + BATCH_SIZE;

        materialTTypes = Z_Realloc(materialTTypes, sizeof(*materialTTypes) * newMax, PU_GAMESTATIC);
        memset(materialTTypes+maxMaterialTTypes, 0, sizeof(*materialTTypes) * (newMax - maxMaterialTTypes));
        maxMaterialTTypes = newMax;
    }

    mtt = &materialTTypes[numMaterialTTypes-1];
    mtt->material = mat;
    mtt->terrainNum = idx - 1;
    return mtt;

#undef BATCH_SIZE
}

void P_InitTerrainTypes(void)
{
    struct matttypedef_s {
        const char* matPath;
        const char* ttName;
    } matTTypeDefs[] =
    {
#if __JDOOM__ || __JDOOM64__
        { MN_FLATS_NAME":FWATER1",  "Water" },
        { MN_FLATS_NAME":LAVA1",    "Lava" },
        { MN_FLATS_NAME":BLOOD1",   "Blood" },
        { MN_FLATS_NAME":NUKAGE1",  "Nukage" },
        { MN_FLATS_NAME":SLIME01",  "Slime" },
#endif
#if __JHERETIC__
        { MN_FLATS_NAME":FLTWAWA1", "Water" },
        { MN_FLATS_NAME":FLTFLWW1", "Water" },
        { MN_FLATS_NAME":FLTLAVA1", "Lava" },
        { MN_FLATS_NAME":FLATHUH1", "Lava" },
        { MN_FLATS_NAME":FLTSLUD1", "Sludge" },
#endif
#if __JHEXEN__
        { MN_FLATS_NAME":X_005",    "Water" },
        { MN_FLATS_NAME":X_001",    "Lava" },
        { MN_FLATS_NAME":X_009",    "Sludge" },
        { MN_FLATS_NAME":F_033",    "Ice" },
#endif
        { 0, 0 }
    };

    if(materialTTypes)
        Z_Free(materialTTypes);
    materialTTypes = 0;
    numMaterialTTypes = maxMaterialTTypes = 0;

    { uint i;
    for(i = 0; matTTypeDefs[i].matPath; ++i)
    {
        uint idx = findTerrainTypeNumForName(matTTypeDefs[i].ttName);
        if(idx)
        {
            material_t* mat = P_ToPtr(DMU_MATERIAL, Materials_IndexForName(matTTypeDefs[i].matPath));
            if(mat)
            {
                VERBOSE( Con_Message("P_InitTerrainTypes: Material \"%s\" linked to terrain type '%s'.\n",
                            matTTypeDefs[i].matPath, matTTypeDefs[i].ttName) )
                getMaterialTerrainType(mat, idx);
            }
        }
    }}
}

void P_ClearTerrainTypes(void)
{
    numMaterialTTypes = 0;
}

void P_ShutdownTerrainTypes(void)
{
    if(materialTTypes)
        Z_Free(materialTTypes);
    materialTTypes = 0;
    numMaterialTTypes = maxMaterialTTypes = 0;
}

/**
 * Return the terrain type of the specified material.
 *
 * @param num           The material to check.
 */
const terraintype_t* P_TerrainTypeForMaterial(material_t* mat)
{
    { const terraintype_t* tt;
    if((tt = findTerrainTypeForMaterial(mat)))
        return tt; // Known, return it.
    }

    // Return the default type.
    return &terrainTypes[0];
}

