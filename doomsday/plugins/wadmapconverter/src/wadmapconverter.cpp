/**
 * @file wadmapconverter.cpp
 * Doomsday Plugin for converting DOOM-like format maps.
 *
 * The purpose of a wadmapconverter plugin is to transform a map into
 * Doomsday's native map format by use of the public map editing interface.
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

#include "wadmapconverter.h"
#include <de/c_wrapper.h>
#include "maplumpinfo.h"
#include "map.h"

int DENG_PLUGIN_GLOBAL(verbose);

map_t DENG_PLUGIN_GLOBAL(theMap);
map_t* DENG_PLUGIN_GLOBAL(map) = &DENG_PLUGIN_GLOBAL(theMap);

static void configure(void)
{
    DENG_PLUGIN_GLOBAL(verbose) = CommandLine_Exists("-verbose");
}

static const ddstring_t* mapFormatNameForId(mapformatid_t id)
{
    static const ddstring_t names[1 + NUM_MAPFORMATS] = {
        /* MF_UNKNOWN */ { "Unknown" },
        /* MF_DOOM    */ { "Doom" },
        /* MF_HEXEN   */ { "Hexen" },
        /* MF_DOOM64  */ { "Doom64" }
    };
    if(VALID_MAPFORMATID(id))
    {
        return &names[1+id];
    }
    return &names[0];
}

static MapLumpType mapLumpTypeForName(const char* name)
{
    static const struct maplumpinfo_s {
        const char* name;
        MapLumpType type;
    } lumptypeForNameDict[] =
    {
        { "THINGS",     ML_THINGS },
        { "LINEDEFS",   ML_LINEDEFS },
        { "SIDEDEFS",   ML_SIDEDEFS },
        { "VERTEXES",   ML_VERTEXES },
        { "SEGS",       ML_SEGS },
        { "SSECTORS",   ML_SSECTORS },
        { "NODES",      ML_NODES },
        { "SECTORS",    ML_SECTORS },
        { "REJECT",     ML_REJECT },
        { "BLOCKMAP",   ML_BLOCKMAP },
        { "BEHAVIOR",   ML_BEHAVIOR },
        { "SCRIPTS",    ML_SCRIPTS },
        { "LIGHTS",     ML_LIGHTS },
        { "MACROS",     ML_MACROS },
        { "LEAFS",      ML_LEAFS },
        { "GL_VERT",    ML_GLVERT },
        { "GL_SEGS",    ML_GLSEGS },
        { "GL_SSECT",   ML_GLSSECT },
        { "GL_NODES",   ML_GLNODES },
        { "GL_PVS",     ML_GLPVS},
        { NULL }
    };

    DENG_ASSERT(name);

    if(name[0])
    for(int i = 0; lumptypeForNameDict[i].name; ++i)
    {
        if(!strnicmp(lumptypeForNameDict[i].name, name, strlen(lumptypeForNameDict[i].name)))
            return lumptypeForNameDict[i].type;
    }

    return ML_INVALID;
}

/**
 * Allocate and initialize a new MapLumpInfo record.
 */
static MapLumpInfo* newMapLumpInfo(lumpnum_t lumpNum, MapLumpType lumpType)
{
    MapLumpInfo* info = static_cast<MapLumpInfo*>(malloc(sizeof(*info)));
    return info->Init(lumpNum, lumpType, W_LumpLength(lumpNum));
}

static lumpnum_t locateMapMarkerLumpForUri(const Uri* uri)
{
    DENG_ASSERT(uri);
    const char* mapId = Str_Text(Uri_Path(uri));
    return W_CheckLumpNumForName2(mapId, true /*quiet please*/);
}

static MapLumpType recogniseMapLump(lumpnum_t lumpNum)
{
    /// @todo Relocate recognition logic from IsSupportedFormat() here.
    return mapLumpTypeForName(W_LumpName(lumpNum));
}

/**
 * Find the lumps associated with this map dataset and link them to the
 * archivedmap record.
 *
 * @note Some obscure PWADs have these lumps in a non-standard order,
 * so we need to go resort to finding them automatically.
 *
 * @param lumpInfos     The MapLumpInfo dictionary to populate.
 * @param startLump     The lump number to begin our search with.
 */
static void collectMapLumps(MapLumpInfo* lumpInfos[NUM_MAPLUMP_TYPES], lumpnum_t startLump)
{
    DENG_ASSERT(lumpInfos);

    WADMAPCONVERTER_TRACE("Locating data lumps...");

    if(startLump < 0) return;

    // Keep checking lumps to see if its a map data lump.
    const int numLumps = *reinterpret_cast<int*>(DD_GetVariable(DD_NUMLUMPS));
    for(lumpnum_t i = startLump; i < numLumps; ++i)
    {
        MapLumpType lumpType = recogniseMapLump(i);

        // If this lump is of an invalid type then we *should* have found all
        // the required map data lumps.
        if(lumpType == ML_INVALID)
        {
            break; // Stop looking.
        }

        // A recognised map data lump; record it in the collection.
        if(lumpInfos[lumpType])
        {
            free(lumpInfos[lumpType]);
        }
        lumpInfos[lumpType] = newMapLumpInfo(i, lumpType);
    }
}

static mapformatid_t recogniseMapFormat(MapLumpInfo* lumpInfos[NUM_MAPLUMP_TYPES])
{
    DENG_ASSERT(lumpInfos);
    if(!IsSupportedFormat(lumpInfos)) return MF_UNKNOWN;
    return DENG_PLUGIN_GLOBAL(map)->format;
}

/**
 * This function will be called when Doomsday is asked to load a map that is
 * not available in its native map format.
 *
 * Our job is to read in the map data structures then use the Doomsday map
 * editing interface to recreate the map in native format.
 */
int ConvertMapHook(int hookType, int parm, void* context)
{
    DENG_UNUSED(hookType);
    DENG_UNUSED(parm);
    DENG_ASSERT(context);

    // Setup the processing parameters.
    configure();

    // Begin the conversion attempt.
    int ret_val = true; // Assume success.

    // Attempt to locate the identified map data marker lump.
    const Uri* uri = reinterpret_cast<const Uri*>(context);
    lumpnum_t markerLump = locateMapMarkerLumpForUri(uri);
    if(0 > markerLump)
    {
        ret_val = false;
        goto FAIL_LOCATE_MAP;
    }

    // Collect all of the map data lumps associated with this map.
    MapLumpInfo* lumpInfos[NUM_MAPLUMP_TYPES];
    memset(lumpInfos, 0, sizeof(lumpInfos));
    collectMapLumps(lumpInfos, markerLump + 1 /*begin after the marker*/);

    memset(DENG_PLUGIN_GLOBAL(map), 0, sizeof(*DENG_PLUGIN_GLOBAL(map)));

    // Do we recognise this format?
    mapformatid_t mapFormat = recogniseMapFormat(lumpInfos);
    if(mapFormat == MF_UNKNOWN)
    {
        ret_val = false;
        goto FAIL_UNKNOWN_FORMAT;
    }

    VERBOSE( Con_Message("WadMapConverter: Recognised a %s format map.\n",
                         Str_Text(mapFormatNameForId(mapFormat))) );

    // Read the archived map.
    int loadError = !LoadMap(lumpInfos);
    if(loadError)
    {
        Con_Message("WadMapConverter: Internal error, aborting conversion...\n");
        ret_val = false;
        goto FAIL_LOAD_ERROR;
    }

    // Perform post read analses.
    AnalyzeMap();

    // Rebuild the map in Doomsday's native format.
    ret_val = TransferMap();

    // Cleanup.
FAIL_LOAD_ERROR:
FAIL_UNKNOWN_FORMAT:
    for(uint i = 0; i < (uint)NUM_MAPLUMP_TYPES; ++i)
    {
        MapLumpInfo* info = lumpInfos[i];
        if(!info) continue;
        free(info);
    }

FAIL_LOCATE_MAP:

    return ret_val;
}

/**
 * This function is called automatically when the plugin is loaded.
 * We let the engine know what we'd like to do.
 */
void DP_Initialize(void)
{
    Plug_AddHook(HOOK_MAP_CONVERT, ConvertMapHook);
}