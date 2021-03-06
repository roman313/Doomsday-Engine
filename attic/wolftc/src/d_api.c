/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2006-2008 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2008 Daniel Swanson <danij@dengine.net>
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
 * d_api.h: Doomsday API setup and interaction - WolfTC specific
 */

// HEADER FILES ------------------------------------------------------------

#include <string.h>

#include "wolftc.h"

#include "d_netsv.h"
#include "d_net.h"
#include "hu_menu.h"
#include "xgclass.h"
#include "g_update.h"
#include "p_mapsetup.h"
#include "p_tick.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// The interface to the Doomsday engine.
game_import_t gi;
game_export_t gx;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

/**
 * Get a 32-bit integer value.
 */
int G_GetInteger(int id)
{
    switch(id)
    {
    case DD_GAME_DMUAPI_VER:
        return DMUAPI_VER;

    default:
        break;
    }
    // ID not recognized, return NULL.
    return 0;
}

/**
 * Get a pointer to the value of a variable. Added for 64-bit support.
 */
void *G_GetVariable(int id)
{
    static float bob[2];

    switch(id)
    {
    case DD_GAME_NAME:
        return GAMENAMETEXT;

    case DD_GAME_NICENAME:
        return GAME_NICENAME;

    case DD_GAME_ID:
        return GAMENAMETEXT " " GAME_VERSION_TEXT;

    case DD_GAME_MODE:
        return gameModeString;

    case DD_GAME_CONFIG:
        return gameConfigString;

    case DD_VERSION_SHORT:
        return GAME_VERSION_TEXT;

    case DD_VERSION_LONG:
        return GAME_VERSION_TEXTLONG "\n" GAME_DETAILS;

    case DD_ACTION_LINK:
        return actionlinks;

    case DD_XGFUNC_LINK:
        return xgClasses;

    case DD_PSPRITE_BOB_X:
        bob[VX] = 1 + (cfg.bobWeapon * players[CONSOLEPLAYER].bob) *
            FIX2FLT(finecosine[(128 * mapTime) & FINEMASK]);

        return &bob[VX];

    case DD_PSPRITE_BOB_Y:
        bob[VY] = 32 + (cfg.bobWeapon * players[CONSOLEPLAYER].bob) *
            FIX2FLT(finesine[(128 * mapTime) & FINEMASK & (FINEANGLES / 2 - 1)]);

        return &bob[VY];

    default:
        break;
    }
    // ID not recognized, return NULL.
    return 0;
}

/**
 * Takes a copy of the engine's entry points and exported data. Returns
 * a pointer to the structure that contains our entry points and exports.
 */
game_export_t *GetGameAPI(game_import_t *imports)
{
    // Take a copy of the imports, but only copy as much data as is
    // allowed and legal.
    memset(&gi, 0, sizeof(gi));
    memcpy(&gi, imports, MIN_OF(sizeof(game_import_t), imports->apiSize));

    // Clear all of our exports.
    memset(&gx, 0, sizeof(gx));

    // Fill in the data for the exports.
    gx.apiSize = sizeof(gx);
    gx.PreInit = G_PreInit;
    gx.PostInit = G_PostInit;
    gx.Shutdown = G_Shutdown;
    gx.Ticker = G_Ticker;
    gx.G_Drawer = D_Display;
    gx.G_Drawer2 = D_Display2;
    gx.PrivilegedResponder = (boolean (*)(event_t *)) G_PrivilegedResponder;
    gx.FallbackResponder = Hu_MenuResponder;
    gx.G_Responder = G_Responder;
    gx.MobjThinker = P_MobjThinker;
    gx.MobjFriction = (float (*)(void *)) P_MobjGetFriction;
    gx.EndFrame = G_EndFrame;
    gx.ConsoleBackground = D_ConsoleBg;
    gx.UpdateState = G_UpdateState;
#undef Get
    gx.GetInteger = G_GetInteger;
    gx.GetVariable = G_GetVariable;

    gx.NetServerStart = D_NetServerStarted;
    gx.NetServerStop = D_NetServerClose;
    gx.NetConnect = D_NetConnect;
    gx.NetDisconnect = D_NetDisconnect;
    gx.NetPlayerEvent = D_NetPlayerEvent;
    gx.NetWorldEvent = D_NetWorldEvent;
    gx.HandlePacket = D_HandlePacket;
    gx.NetWriteCommands = D_NetWriteCommands;
    gx.NetReadCommands = D_NetReadCommands;

    // Data structure sizes.
    gx.ticcmdSize = sizeof(ticcmd_t);
    gx.mobjSize = sizeof(mobj_t);
    gx.polyobjSize = sizeof(polyobj_t);

    gx.SetupForMapData = P_SetupForMapData;

    // These really need better names. Ideas?
    gx.HandleMapDataPropertyValue = P_HandleMapDataPropertyValue;
    gx.HandleMapObjectStatusReport = P_HandleMapObjectStatusReport;
    return &gx;
}
