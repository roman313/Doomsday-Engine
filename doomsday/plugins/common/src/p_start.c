/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2009 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2009 Daniel Swanson <danij@dengine.net>
 *\author Copyright © 1999 Activision
 *\author Copyright © 1993-1996 by id Software, Inc.
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
 * p_start.c
 */

// HEADER FILES ------------------------------------------------------------

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#if __JDOOM__
#  include "jdoom.h"
#  include "r_common.h"
#  include "hu_stuff.h"
#elif __JDOOM64__
#  include "jdoom64.h"
#  include "r_common.h"
#  include "hu_stuff.h"
#elif __JHERETIC__
#  include "jheretic.h"
#  include "r_common.h"
#  include "hu_stuff.h"
#elif __JHEXEN__
#  include "jhexen.h"
#endif

#include "p_tick.h"
#include "p_mapsetup.h"
#include "p_user.h"
#include "d_net.h"
#include "p_map.h"
#include "p_terraintype.h"
#include "g_common.h"
#include "p_start.h"
#include "p_actor.h"
#include "p_switch.h"
#include "g_defs.h"
#include "p_inventory.h"

// MACROS ------------------------------------------------------------------

#if __JDOOM__ || __JDOOM64__ || __JHERETIC__
#  define TELEPORTSOUND     SFX_TELEPT
#  define MAX_START_SPOTS   4 // Maximum number of different player starts.
#else
#  define TELEPORTSOUND     SFX_TELEPORT
#  define MAX_START_SPOTS   8
#endif

// Time interval for item respawning.
#define SPAWNQUEUE_MAX         128

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

uint numMapSpots;
mapspot_t* mapSpots;

#if __JHERETIC__
int maceSpotCount;
mapspot_t* maceSpots;
int bossSpotCount;
mapspot_t* bossSpots;
#endif

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int numPlayerStarts = 0;
static playerstart_t* playerStarts;
static int numPlayerDMStarts = 0;
static playerstart_t* deathmatchStarts;

// CODE --------------------------------------------------------------------

static boolean fuzzySpawnPosition(float* x, float* y, float* z,
                                  angle_t* angle, int* spawnFlags)
{
#define XOFFSET         (33) // Player radius = 16
#define YOFFSET         (33) // Player radius = 16

    int                 i;

    assert(x);
    assert(y);

    // Try some spots in the vicinity.
    for(i = 0; i < 9; ++i)
    {
        float               pos[2];

        pos[VX] = *x;
        pos[VY] = *y;

        if(i != 0)
        {
            int                 k = (i == 4 ? 0 : i);

            // Move a bit.
            pos[VX] += (k % 3 - 1) * XOFFSET;
            pos[VY] += (k / 3 - 1) * YOFFSET;
        }

        if(P_CheckSpot(pos[VX], pos[VY]))
        {
            *x = pos[VX];
            *y = pos[VY];
            return true;
        }
    }

#undef XOFFSET
#undef YOFFSET

    return false;
}

/**
 * Given a doomednum, look up the associated mobj type.
 *
 * @param doomEdNum     Doom Editor (Thing) Number to look up.
 * @return              The associated mobj type if found else @c MT_NONE.
 */
mobjtype_t P_DoomEdNumToMobjType(int doomEdNum)
{
    int                 i;

    for(i = 0; i < Get(DD_NUMMOBJTYPES); ++i)
    {
        if(doomEdNum == MOBJINFO[i].doomEdNum)
            return i;
    }

    return MT_NONE;
}

/**
 * Initializes various playsim related data
 */
void P_Init(void)
{
#if __JHERETIC__ || __JHEXEN__ || __JDOOM64__
    P_InitInventory();
#endif

#if __JHEXEN__
    P_InitMapInfo();
#endif

    P_InitSwitchList();
    P_InitPicAnims();

    P_InitTerrainTypes();
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    P_InitLava();
#endif

    maxHealth = 100;
    GetDefInt("Player|Max Health", &maxHealth);

#if __JDOOM__ || __JDOOM64__
    healthLimit = 200;
    godModeHealth = 100;
    megaSphereHealth = 200;
    soulSphereHealth = 100;
    soulSphereLimit = 200;

    armorPoints[0] = 100;
    armorPoints[1] = armorPoints[2] = armorPoints[3] = 200;
    armorClass[0] = 1;
    armorClass[1] = armorClass[2] = armorClass[3] = 2;

    GetDefInt("Player|Health Limit", &healthLimit);
    GetDefInt("Player|God Health", &godModeHealth);

    GetDefInt("Player|Green Armor", &armorPoints[0]);
    GetDefInt("Player|Blue Armor", &armorPoints[1]);
    GetDefInt("Player|IDFA Armor", &armorPoints[2]);
    GetDefInt("Player|IDKFA Armor", &armorPoints[3]);

    GetDefInt("Player|Green Armor Class", &armorClass[0]);
    GetDefInt("Player|Blue Armor Class", &armorClass[1]);
    GetDefInt("Player|IDFA Armor Class", &armorClass[2]);
    GetDefInt("Player|IDKFA Armor Class", &armorClass[3]);

    GetDefInt("MegaSphere|Give|Health", &megaSphereHealth);

    GetDefInt("SoulSphere|Give|Health", &soulSphereHealth);
    GetDefInt("SoulSphere|Give|Health Limit", &soulSphereLimit);
#endif
}

void P_CreatePlayerStart(int defaultPlrNum, byte entryPoint,
                         boolean deathmatch, float x, float y, float z,
                         angle_t angle, int spawnFlags)
{
    playerstart_t*      start;

    if(deathmatch)
    {
        deathmatchStarts = Z_Realloc(deathmatchStarts,
            sizeof(playerstart_t) * ++numPlayerDMStarts, PU_MAP);
        start = &deathmatchStarts[numPlayerDMStarts - 1];
    }
    else
    {
        playerStarts = Z_Realloc(playerStarts,
            sizeof(playerstart_t) * ++numPlayerStarts, PU_MAP);
        start = &playerStarts[numPlayerStarts - 1];
    }

    start->plrNum = defaultPlrNum;
    start->entryPoint = entryPoint;
    start->pos[VX] = x;
    start->pos[VY] = y;
    start->pos[VZ] = z;
    start->angle = angle;
    start->spawnFlags = spawnFlags;
}

void P_DestroyPlayerStarts(void)
{
    if(playerStarts)
        Z_Free(playerStarts);
    playerStarts = NULL;
    numPlayerStarts = 0;

    if(deathmatchStarts)
        Z_Free(deathmatchStarts);
    deathmatchStarts = NULL;
    numPlayerDMStarts = 0;
}

/**
 * @return              The correct start for the player. The start is in
 *                      the given group for specified entry point.
 */
const playerstart_t* P_GetPlayerStart(byte entryPoint, int pnum,
                                      boolean deathmatch)
{
#if __JHEXEN__
    int                 i;
    const playerstart_t* def = NULL;
#endif

    if((deathmatch && !numPlayerDMStarts) || !numPlayerStarts)
        return NULL;

    if(pnum < 0)
        pnum = P_Random() % (deathmatch? numPlayerDMStarts:numPlayerStarts);
    else
        pnum = MINMAX_OF(0, pnum, MAXPLAYERS-1);

    if(deathmatch)
    {   // In deathmatch, entry point is ignored.
        return &deathmatchStarts[pnum];
    }

#if __JHEXEN__
    for(i = 0; i < numPlayerStarts; ++i)
    {
        const playerstart_t* start = &playerStarts[i];

        if(start->entryPoint == entryPoint && start->plrNum - 1 == pnum)
            return start;
        if(!start->entryPoint && start->plrNum - 1 == pnum)
            def = start;
    }

    // Return the default choice.
    return def;
#else
    return &playerStarts[players[pnum].startSpot];
#endif
}

uint P_GetNumPlayerStarts(boolean deathmatch)
{
    if(deathmatch)
        return numPlayerDMStarts;

    return numPlayerStarts;
}

/**
 * Gives all the players in the game a playerstart.
 * Only needed in co-op games (start spots are random in deathmatch).
 */
void P_DealPlayerStarts(byte entryPoint)
{
    int                 i;

    if(!numPlayerStarts)
    {
        Con_Message("P_DealPlayerStarts: Warning, no player starts!\n");
        return;
    }

    // First assign one start per player, only accepting perfect matches.
    for(i = 0; i < MAXPLAYERS; ++i)
    {
        int                 k, spotNumber;
        player_t*           pl = &players[i];

        if(!pl->plr->inGame)
            continue;

        // The number of the start spot this player will use.
        spotNumber = i % MAX_START_SPOTS;
        pl->startSpot = -1;

        for(k = 0; k < numPlayerStarts; ++k)
        {
            const playerstart_t* start = &playerStarts[k];

            if(spotNumber == start->plrNum - 1 &&
               start->entryPoint == entryPoint)
            {   // A match!
                pl->startSpot = k;
                // Keep looking.
            }
        }

        // If still without a start spot, assign one randomly.
        if(pl->startSpot == -1)
        {
            // It's likely that some players will get the same start spots.
            pl->startSpot = M_Random() % numPlayerStarts;
        }
    }

    if(IS_NETGAME)
    {
        Con_Printf("Player starting spots:\n");
        for(i = 0; i < MAXPLAYERS; ++i)
        {
            player_t*           pl = &players[i];

            if(!pl->plr->inGame)
                continue;

            Con_Printf("- pl%i: color %i, spot %i\n", i, cfg.playerColor[i],
                       pl->startSpot);
        }
    }
}

/**
 * Called when a player is spawned into the map. Most of the player
 * structure stays unchanged between maps.
 */
void P_SpawnPlayer(int plrNum, float x, float y, float z, angle_t angle,
                   int spawnFlags, boolean makeCamera)
{
#if __JHEXEN__
    playerclass_t       pClass;

    if(randomClassParm && deathmatch)
    {
        pClass = P_Random() % 3;
        if(pClass == cfg.playerClass[plrNum])
            pClass = (pClass + 1) % 3;
    }
    else
    {
        pClass = cfg.playerClass[plrNum];
    }
#else
    playerclass_t       pClass = PCLASS_PLAYER;
#endif

    P_SpawnPlayer2(plrNum, pClass, x, y, z, angle, spawnFlags, makeCamera);
}

static void spawnPlayer(int plrNum, float x, float y, float z,
                        angle_t angle, int spawnFlags, boolean makeCamera,
                        boolean doTeleSpark, boolean doTeleFrag)
{
    player_t*           plr;
#if __JDOOM__ || __JDOOM64__
    boolean             queueBody = (plrNum >= 0? true : false);
#endif

    /* $voodoodolls */
    if(plrNum < 0)
        plrNum = -plrNum - 1;
    plrNum = MINMAX_OF(0, plrNum, MAXPLAYERS-1);

    plr = &players[plrNum];

#if __JDOOM__ || __JDOOM64__
    if(queueBody)
        G_QueueBody(plr->plr->mo);
#endif

    P_SpawnPlayer(plrNum, x, y, z, angle, spawnFlags, makeCamera);

    // Spawn a teleport fog?
    if(doTeleSpark && !makeCamera)
    {
        mobj_t*             mo;
        uint                an = angle >> ANGLETOFINESHIFT;

        x += 20 * FIX2FLT(finecosine[an]);
        y += 20 * FIX2FLT(finesine[an]);

        if((mo = P_SpawnTeleFog(x, y, angle + ANG180)))
        {
            // Don't start sound on first frame.
            if(mapTime > 1)
                S_StartSound(TELEPORTSOUND, mo);
        }
    }

    // Camera players do not telefrag.
    if(!makeCamera && doTeleFrag)
        P_Telefrag(plr->plr->mo);
}

/**
 * Called by G_DoReborn if playing a net game.
 */
void P_RebornPlayer(int plrNum)
{
#if __JHEXEN__
    int                 i, oldKeys, oldPieces, bestWeapon;
    boolean             oldWeaponOwned[NUM_WEAPON_TYPES];
#endif
    boolean             foundSpot;
    const playerstart_t* assigned;
    player_t*           p;

    if(plrNum < 0 || plrNum >= MAXPLAYERS)
        return; // Wha?

    p = &players[plrNum];

    Con_Printf("P_RebornPlayer: %i.\n", plrNum);

    if(p->plr->mo)
    {
        // First dissasociate the corpse.
        p->plr->mo->player = NULL;
        p->plr->mo->dPlayer = NULL;
    }

    if(IS_CLIENT)
    {
        if(G_GetGameState() == GS_MAP)
        {
            // Anywhere will do, for now.
            spawnPlayer(plrNum, 0, 0, 0, 0, MSF_Z_FLOOR, false, false, false);
        }

        return;
    }

    // Spawn at random spot if in death match.
    if(deathmatch)
    {
        G_DeathMatchSpawnPlayer(plrNum);
        return;
    }

#if __JHEXEN__
    // Cooperative net-play, retain keys and weapons
    oldKeys = p->keys;
    oldPieces = p->pieces;
    for(i = 0; i < NUM_WEAPON_TYPES; ++i)
        oldWeaponOwned[i] = p->weapons[i].owned;
#endif

    // Try to spawn at the assigned spot.
    foundSpot = false;
    assigned = P_GetPlayerStart(
#if __JHEXEN__
                                rebornPosition,
#else
                                0,
#endif
                                plrNum, false);

    if(P_CheckSpot(assigned->pos[VX], assigned->pos[VY]))
    {   // Appropriate player start spot is open.
#if __JDOOM__ || __JDOOM64__
        G_QueueBody(players[plrNum].plr->mo);
#endif

        Con_Printf("- spawning at assigned spot\n");
        P_SpawnPlayer(plrNum, assigned->pos[VX], assigned->pos[VY],
                      assigned->pos[VZ], assigned->angle,
                      assigned->spawnFlags, false);
        // Spawn a teleport fog
        {
        mobj_t*             mo;
        uint                an = assigned->angle >> ANGLETOFINESHIFT;
        float               pos[2];

        pos[VX] = assigned->pos[VX] + 20 * FIX2FLT(finecosine[an]);
        pos[VY] = assigned->pos[VY] + 20 * FIX2FLT(finesine[an]);

        if((mo = P_SpawnTeleFog(pos[VX], pos[VY], assigned->angle+ANG180)))
        {
            // Don't start sound on first frame.
            if(mapTime > 1)
                S_StartSound(TELEPORTSOUND, mo);
        }
        }

        foundSpot = true;
    }
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    else
    {
        float               pos[3];
        angle_t             angle;
        int                 spawnFlags;
        boolean             makeCamera;

        if(assigned)
        {
            pos[VX] = assigned->pos[VX];
            pos[VY] = assigned->pos[VY];
            pos[VZ] = assigned->pos[VZ];
            angle = assigned->angle;
            spawnFlags = assigned->spawnFlags;

            // "Fuzz" the spawn position looking for room nearby.
            makeCamera = !fuzzySpawnPosition(&pos[VX], &pos[VY], &pos[VZ],
                                             &angle, &spawnFlags);
        }
        else
        {
            pos[VX] = pos[VY] = pos[VZ] = 0;
            angle = 0;
            spawnFlags = MSF_Z_FLOOR;
            makeCamera = true;
        }

        Con_Printf("- force spawning at %i.\n", p->startSpot);

        spawnPlayer(plrNum, pos[VX], pos[VY], pos[VZ], angle, spawnFlags,
                    makeCamera, true, true);
    }
#else
    else
    {
        // Try to spawn at one of the other player start spots.
        for(i = 0; i < MAXPLAYERS; ++i)
        {
            const playerstart_t* start;

            if((start = P_GetPlayerStart(rebornPosition, i, false)))
            {
                if(P_CheckSpot(start->pos[VX], start->pos[VY]))
                {
                    // Found an open start spot.
                    spawnPlayer(i, start->pos[VX], start->pos[VY],
                                start->pos[VZ], start->angle,
                                start->spawnFlags, false, true, false);
                    foundSpot = true;
                    break;
                }
            }
        }
    }

    if(!foundSpot)
    {   // Player's going to be inside something.
        const playerstart_t* start;

        if((start = P_GetPlayerStart(rebornPosition, plrNum, false)))
        {
            spawnPlayer(plrNum, start->pos[VX], start->pos[VY],
                        start->pos[VZ], start->angle, start->spawnFlags,
                        false, false, false);
        }
        else
        {
            spawnPlayer(plrNum, 0, 0, 0, 0, MSF_Z_FLOOR, true, false, false);
        }
    }

    // Restore keys and weapons
    p->keys = oldKeys;
    p->pieces = oldPieces;
    for(bestWeapon = 0, i = 0; i < NUM_WEAPON_TYPES; ++i)
    {
        if(oldWeaponOwned[i])
        {
            bestWeapon = i;
            p->weapons[i].owned = true;
        }
    }

    p->ammo[AT_BLUEMANA].owned = 25; //// \fixme values.ded
    p->ammo[AT_GREENMANA].owned = 25; //// \fixme values.ded
    if(bestWeapon)
    {   // Bring up the best weapon.
        p->pendingWeapon = bestWeapon;
    }
#endif
}

/**
 * @return              @c false if the player cannot be respawned at the
 *                      given location because something is occupying it.
 */
boolean P_CheckSpot(float x, float y)
{
#if __JHEXEN__
#define DUMMY_TYPE      MT_PLAYER_FIGHTER
#else
#define DUMMY_TYPE      MT_PLAYER
#endif

    float               pos[3];
    mobj_t*             dummy;
    boolean             result;

    pos[VX] = x;
    pos[VY] = y;
    pos[VZ] = 0;

    // Create a dummy to test with.
    if(!(dummy = P_SpawnMobj3fv(DUMMY_TYPE, pos, 0, MSF_Z_FLOOR)))
        Con_Error("P_CheckSpot: Failed creating dummy mobj.");

    dummy->flags2 &= ~MF2_PASSMOBJ;

    result = P_CheckPosition3fv(dummy, pos);

    P_MobjRemove(dummy, true);

    return result;

#undef DUMMY_TYPE
}

#if __JHERETIC__
void P_AddMaceSpot(float x, float y, angle_t angle)
{
    mapspot_t*          spot;

    maceSpots = Z_Realloc(maceSpots, sizeof(mapspot_t) * ++maceSpotCount,
                          PU_MAP);
    spot = &maceSpots[maceSpotCount-1];

    spot->pos[VX] = x;
    spot->pos[VY] = y;
    spot->angle = angle;
}

void P_AddBossSpot(float x, float y, angle_t angle)
{
    mapspot_t*          spot;

    bossSpots = Z_Realloc(bossSpots, sizeof(mapspot_t) * ++bossSpotCount,
                          PU_MAP);
    spot = &bossSpots[bossSpotCount-1];

    spot->pos[VX] = x;
    spot->pos[VY] = y;
    spot->angle = angle;
}
#endif

mobj_t* P_SpawnMobjAtSpot(mobjtype_t type, const mapspot_t* spot)
{
    mobj_t*             mo;

    if((mo = P_SpawnMobj3fv(type, spot->pos, spot->angle, spot->flags)))
    {
        if(mo->tics > 0)
            mo->tics = 1 + (P_Random() % mo->tics);

#if __JHEXEN__
        mo->tid = spot->tid;
        mo->special = spot->special;
        mo->args[0] = spot->arg1;
        mo->args[1] = spot->arg2;
        mo->args[2] = spot->arg3;
        mo->args[3] = spot->arg4;
        mo->args[4] = spot->arg5;
#endif

#if __JHEXEN__
        if(mo->flags2 & MF2_FLOATBOB)
            mo->special1 = FLT2FIX(spot->pos[VZ]);
#endif

#if __JDOOM__ || __JDOOM64__ || __JHERETIC__
        if(mo->flags & MF_COUNTKILL)
            totalKills++;
        if(mo->flags & MF_COUNTITEM)
            totalItems++;
#endif

#if __JHEXEN__
        if(spot->flags & MTF_DORMANT)
        {
            mo->flags2 |= MF2_DORMANT;
            if(mo->type == MT_ICEGUY)
            {
                P_MobjChangeState(mo, S_ICEGUY_DORMANT);
            }
            mo->tics = -1;
        }
#endif

#if __JDOOM64__
        /*if(spot->flags & MTF_WALKOFF)
            mo->flags |= (MF_FLOAT | MF_DROPOFF);

        if(spot->flags & MTF_TRANSLUCENT)
            mo->flags |= MF_SHADOW;

        if(spot->flags & MTF_FLOAT)
        {
            mo->pos[VZ] += 96;
            mo->flags |= (MF_FLOAT | MF_NOGRAVITY);
        }*/
#endif
    }

    return mo;
}

/**
 * Spawns the passed thing into the world.
 */
static void spawnMapThing(const mapspot_t* spot)
{
#if __JHEXEN__
    static unsigned int classFlags[] = {
        MTF_FIGHTER,
        MTF_CLERIC,
        MTF_MAGE
    };
#endif

    int                 spawnMask;
    mobjtype_t          type;
    const mobjinfo_t*   info;

/*#if _DEBUG
Con_Message("spawnMapThing: x:[%g, %g, %g] angle:%i ednum:%i flags:%i\n",
            spot->pos[VX], spot->pos[VY], spot->pos[VZ], spot->angle,
            spot->doomedNum, spot->flags);
#endif*/

    if(spot->doomEdNum >= 1 && spot->doomEdNum <= 4) // Player starts 1 to 4.
        return;

#if __JHEXEN__
    if(spot->doomEdNum >= 9100 && spot->doomEdNum <= 9103) // Player starts 5 to 8.
        return;
#endif

    if(spot->doomEdNum == 11) // Player starts (deathmatch).
        return;

#if __JHERETIC__
    // Ambient sound origin?
    if(spot->doomEdNum >= 1200 && spot->doomEdNum < 1300)
        return;

    if(spot->doomEdNum == 56) // Boss spot.
        return;

    if(spot->doomEdNum == 2002) // Mace spot.
        return;
#endif

#if __JHEXEN__
    // Sound sequence origin?
    if(spot->doomEdNum >= 1400 && spot->doomEdNum < 1410)
        return;
#endif

#if __JDOOM__ || __JDOOM64__ || __JHERETIC
    // Don't spawn things flagged for Multiplayer if we're not in a netgame.
    if(!IS_NETGAME && (spot->flags & MSF_NOTSINGLE))
        return;
#endif

#if __JDOOM__
    // Don't spawn things flagged for Not Deathmatch if we're deathmatching.
    if(deathmatch && (spot->flags & MSF_NOTDM))
        return;

    // Don't spawn things flagged for Not Coop if we're coop'in.
    if(IS_NETGAME && !deathmatch && (spot->flags & MSF_NOTCOOP))
        return;

    // Don't spawn things flagged for Multiplayer if we're not in a netgame.
    if(!IS_NETGAME && (spot->flags & MSF_NOTSINGLE))
        return;
#endif

#if __JHEXEN__
    // Check current game type with spawn flags.
    if(IS_NETGAME == false)
        spawnMask = MTF_GSINGLE;
    else if(deathmatch)
        spawnMask = MTF_GDEATHMATCH;
    else
        spawnMask = MTF_GCOOP;

    if(!(spot->flags & spawnMask))
        return;
#endif

    // Check for apropriate skill level.
#if __JHEXEN__
    if(gameSkill == SM_BABY || gameSkill == SM_EASY)
        spawnMask = MSF_EASY;
    else if(gameSkill == SM_HARD || gameSkill == SM_NIGHTMARE)
        spawnMask = MSF_HARD;
    else
        spawnMask = MSF_MEDIUM;
#else
    if(gameSkill == SM_BABY)
        spawnMask = 1;
# if __JDOOM__
    else if(gameSkill == SM_NIGHTMARE)
        spawnMask = 4;
# endif
    else
        spawnMask = 1 << (gameSkill - 1);
#endif

    if(!(spot->flags & spawnMask))
        return;

#if __JHEXEN__
    // Check current character classes with spawn flags.
    if(IS_NETGAME == false)
    {   // Single player.
        if((spot->flags & classFlags[cfg.playerClass[0]]) == 0)
        {   // Not for current class.
            return;
        }
    }
    else if(deathmatch == false)
    {   // Cooperative.
        int                 i;

        spawnMask = 0;
        for(i = 0; i < MAXPLAYERS; ++i)
        {
            if(players[i].plr->inGame)
            {
                spawnMask |= classFlags[cfg.playerClass[i]];
            }
        }

        // No players are in the game when a dedicated server is started.
        // In this case, we'll be generous and spawn stuff for all the
        // classes.
        if(!spawnMask)
        {
            spawnMask |= MTF_FIGHTER | MTF_CLERIC | MTF_MAGE;
        }

        if((spot->flags & spawnMask) == 0)
        {
            return;
        }
    }
#endif

    // Find which type to spawn.
    if((type = P_DoomEdNumToMobjType(spot->doomEdNum)) == MT_NONE)
    {
        Con_Message("spawnMapThing: Warning, unknown thing num %i at "
                    "[%g, %g, %g].\n", spot->doomEdNum, spot->pos[VX],
                    spot->pos[VY], spot->pos[VZ]);
        return;
    }
    info = &MOBJINFO[type];

    // Clients only spawn local objects.
    if(!(info->flags & MF_LOCAL) && IS_CLIENT)
        return;

    // Not for deathmatch?
    if(deathmatch && (info->flags & MF_NOTDMATCH))
        return;

    // Check for specific disabled objects.
#if __JDOOM__ || __JDOOM64__
    if((spot->flags & MSF_NOTSINGLE) && IS_NETGAME)
    {
        // Cooperative weapons?
        if(cfg.noCoopWeapons && !deathmatch && type >= MT_CLIP &&
           type <= MT_SUPERSHOTGUN)
            return;

        // Don't spawn any special objects in coop?
        if(cfg.noCoopAnything && !deathmatch)
            return;

        // BFG disabled in netgames?
        if(cfg.noNetBFG && type == MT_MISC25)
            return;
    }
# if __JDOOM__
    switch(type)
    {
    case MT_SPIDER: // 68, Arachnotron
    case MT_VILE: // 64, Archvile
    case MT_BOSSBRAIN: // 88, Boss Brain
    case MT_BOSSSPIT: // 89, Boss Shooter
    case MT_KNIGHT: // 69, Hell Knight
    case MT_FATSO: // 67, Mancubus
    case MT_PAIN: // 71, Pain Elemental
    case MT_MEGA: // 74, MegaSphere
    case MT_CHAINGUY: // 65, Former Human Commando
    case MT_UNDEAD: // 66, Revenant
    case MT_WOLFSS: // 84, Wolf SS
        if(gameMode != commercial)
            return;
        break;

    default:
        break;
    }
# endif
#elif __JHERETIC__
    switch(type)
    {
    case MT_WSKULLROD:
    case MT_WPHOENIXROD:
    case MT_AMSKRDWIMPY:
    case MT_AMSKRDHEFTY:
    case MT_AMPHRDWIMPY:
    case MT_AMPHRDHEFTY:
    case MT_AMMACEWIMPY:
    case MT_AMMACEHEFTY:
    case MT_ARTISUPERHEAL:
    case MT_ARTITELEPORT:
    case MT_ITEMSHIELD2:
        if(gameMode == shareware)
        {   // Don't place on map in shareware version.
            return;
        }
        break;

    default:
        break;
    }
#elif __JHEXEN__
    switch(type)
    {
    case MT_ZLYNCHED_NOHEART:
        P_SpawnMobj3fv(MT_BLOODPOOL, spot->pos, 0,
                       spot->flags | MSF_Z_FLOOR);
        break;

    default:
        break;
    }
#endif

    // Don't spawn any monsters if -noMonstersParm.
    if(noMonstersParm)
    {
        if((info->flags & MF_COUNTKILL)
#if __JDOOM__ || __JDOOM64__
           || type == MT_SKULL
#endif
           )
            return;
    }

    // Spawn it!
    P_SpawnMobjAtSpot(type, spot);
}

/**
 * Spawns all THINGS that belong in the map.
 *
 * Polyobject anchors etc are still handled in PO_Init()
 */
void P_SpawnThings(void)
{
    uint                i;

    for(i = 0; i < numMapSpots; ++i)
    {
        spawnMapThing(&mapSpots[i]);
    }
}

/**
 * Spawns all players, using the method appropriate for current game mode.
 * Called during map setup.
 */
void P_SpawnPlayers(void)
{
    int                 i;

    // If deathmatch, randomly spawn the active players.
    if(deathmatch)
    {
        for(i = 0; i < MAXPLAYERS; ++i)
            if(players[i].plr->inGame)
            {
                players[i].plr->mo = NULL;
                G_DeathMatchSpawnPlayer(i);
            }
    }
    else
    {
#if __JDOOM__ || __JDOOM64__
        if(!IS_NETGAME)
        {
            /* $voodoodolls */
            for(i = 0; i < numPlayerStarts; ++i)
            {
                if(players[0].startSpot != i && playerStarts[i].plrNum == 1)
                {
                    const playerstart_t* start = &playerStarts[i];

                    spawnPlayer(-1, start->pos[VX], start->pos[VY],
                                start->pos[VZ], start->angle,
                                start->spawnFlags, false, false, false);
                }
            }
        }
#endif
        // Spawn everybody at their assigned places.
        // Might get messy if there aren't enough starts.
        for(i = 0; i < MAXPLAYERS; ++i)
            if(players[i].plr->inGame)
            {
                const playerstart_t* start = NULL;
                ddplayer_t*         ddpl = players[i].plr;
                float               pos[3];
                angle_t             angle;
                int                 spawnFlags;
                boolean             makeCamera;

                if(players[i].startSpot < numPlayerStarts)
                    start = &playerStarts[players[i].startSpot];

                if(start)
                {
                    pos[VX] = start->pos[VX];
                    pos[VY] = start->pos[VY];
                    pos[VZ] = start->pos[VZ];
                    angle = start->angle;
                    spawnFlags = start->spawnFlags;

                    // "Fuzz" the spawn position looking for room nearby.
                    makeCamera = !fuzzySpawnPosition(&pos[VX], &pos[VY],
                        &pos[VZ], &angle, &spawnFlags);
                }
                else
                {
                    pos[VX] = pos[VY] = pos[VZ] = 0;
                    angle = 0;
                    spawnFlags = MSF_Z_FLOOR;
                    makeCamera = true;
                }

                spawnPlayer(i, pos[VX], pos[VY], pos[VZ], angle, spawnFlags,
                            makeCamera, false, true);
            }
    }
}

/**
 * Spawns a player at one of the random death match spots.
 */
void G_DeathMatchSpawnPlayer(int playerNum)
{
    int                 i;
    ddplayer_t*         pl;
    const playerstart_t* start;

    playerNum = MINMAX_OF(0, playerNum, MAXPLAYERS-1);

    // Now let's find an available deathmatch start.
    if(numPlayerDMStarts < 2)
        Con_Error("G_DeathMatchSpawnPlayer: Error, minimum of two "
                  "(deathmatch) mapspots required for deathmatch.");

    pl = players[playerNum].plr;

    for(i = 0; i < 20; ++i)
    {
        start = &deathmatchStarts[P_Random() % numPlayerDMStarts];

        if(P_CheckSpot(start->pos[VX], start->pos[VY]))
            break;
    }

    spawnPlayer(playerNum, start->pos[VX], start->pos[VY], start->pos[VZ],
                start->angle, start->spawnFlags, false, true, true);
}

typedef struct {
    float               pos[2], minDist;
} unstuckmobjinlinedefparams_t;

boolean unstuckMobjInLinedef(linedef_t* li, void* context)
{
    unstuckmobjinlinedefparams_t *params =
        (unstuckmobjinlinedefparams_t*) context;

    if(!P_GetPtrp(li, DMU_BACK_SECTOR))
    {
        float               pos, linePoint[2], lineDelta[2], result[2];

        /**
         * Project the point (mo position) onto this linedef. If the
         * resultant point lies on the linedef and the current position is
         * in range of that point, adjust the position moving it away from
         * the projected point.
         */

        P_GetFloatpv(P_GetPtrp(li, DMU_VERTEX0), DMU_XY, linePoint);
        P_GetFloatpv(li, DMU_DXY, lineDelta);

        pos = M_ProjectPointOnLine(params->pos, linePoint, lineDelta, 0, result);

        if(pos > 0 && pos < 1)
        {
            float               dist =
                P_ApproxDistance(params->pos[VX] - result[VX],
                                 params->pos[VY] - result[VY]);

            if(dist >= 0 && dist < params->minDist)
            {
                float               len, unit[2], normal[2];

                // Derive the line normal.
                len = P_ApproxDistance(lineDelta[0], lineDelta[1]);
                if(len)
                {
                    unit[VX] = lineDelta[0] / len;
                    unit[VY] = lineDelta[1] / len;
                }
                else
                {
                    unit[VX] = unit[VY] = 0;
                }
                normal[VX] =  unit[VY];
                normal[VY] = -unit[VX];

                // Adjust the position.
                params->pos[VX] += normal[VX] * params->minDist;
                params->pos[VY] += normal[VY] * params->minDist;
            }
        }
    }

    return true; // Continue iteration.
}

boolean iterateLinedefsNearMobj(thinker_t* th, void* context)
{
    mobj_t*             mo = (mobj_t*) th;
    mobjtype_t          type = *((mobjtype_t*) context);
    float               aabb[4];
    unstuckmobjinlinedefparams_t params;

    // \todo Why not type-prune at an earlier point? We could specify a
    // custom comparison func for DD_IterateThinkers...
    if(mo->type != type)
        return true; // Continue iteration.

    aabb[BOXLEFT]   = mo->pos[VX] - mo->radius;
    aabb[BOXRIGHT]  = mo->pos[VX] + mo->radius;
    aabb[BOXBOTTOM] = mo->pos[VY] - mo->radius;
    aabb[BOXTOP]    = mo->pos[VY] + mo->radius;

    params.pos[VX] = mo->pos[VX];
    params.pos[VY] = mo->pos[VY];
    params.minDist = mo->radius / 2;

    VALIDCOUNT++;

    P_LinesBoxIterator(aabb, unstuckMobjInLinedef, &params);

    if(mo->pos[VX] != params.pos[VX] || mo->pos[VY] != params.pos[VY])
    {
        mo->angle = R_PointToAngle2(mo->pos[VX], mo->pos[VY],
                                    params.pos[VX], params.pos[VY]);
        P_MobjUnsetPosition(mo);
        mo->pos[VX] = params.pos[VX];
        mo->pos[VY] = params.pos[VY];
        P_MobjSetPosition(mo);
    }

    return true; // Continue iteration.
}

/**
 * Only affects torches, which are often placed inside walls in the
 * original maps. The DOOM engine allowed these kinds of things but a
 * Z-buffer doesn't.
 */
void P_MoveThingsOutOfWalls(void)
{
    static const mobjtype_t types[] = {
#if __JHERETIC__
        MT_MISC10,
#elif __JHEXEN__
        MT_ZWALLTORCH,
        MT_ZWALLTORCH_UNLIT,
#endif
        NUMMOBJTYPES // terminate.
    };
    uint                i;

    for(i = 0; types[i] != NUMMOBJTYPES; ++i)
    {
        mobjtype_t          type = types[i];

        DD_IterateThinkers(P_MobjThinker, iterateLinedefsNearMobj, &type);
    }
}

#if __JHERETIC__
float P_PointLineDistance(linedef_t *line, float x, float y, float *offset)
{
    float   a[2], b[2], c[2], d[2], len;

    P_GetFloatpv(P_GetPtrp(line, DMU_VERTEX0), DMU_XY, a);
    P_GetFloatpv(P_GetPtrp(line, DMU_VERTEX1), DMU_XY, b);

    c[VX] = x;
    c[VY] = y;

    d[VX] = b[VX] - a[VX];
    d[VY] = b[VY] - a[VY];
    len = sqrt(d[VX] * d[VX] + d[VY] * d[VY]);  // Accurate.

    if(offset)
        *offset =
            ((a[VY] - c[VY]) * (a[VY] - b[VY]) -
             (a[VX] - c[VX]) * (b[VX] - a[VX])) / len;
    return ((a[VY] - c[VY]) * (b[VX] - a[VX]) -
            (a[VX] - c[VX]) * (b[VY] - a[VY])) / len;
}

/**
 * Fails in some places, but works most of the time.
 */
void P_TurnGizmosAwayFromDoors(void)
{
#define MAXLIST 200

    sector_t   *sec;
    mobj_t     *iter;
    uint        i, l;
    int         k, t;
    linedef_t     *closestline = NULL, *li;
    xline_t    *xli;
    float       closestdist = 0, dist, off, linelen;    //, minrad;
    mobj_t     *tlist[MAXLIST];

    for(i = 0; i < numsectors; ++i)
    {
        sec = P_ToPtr(DMU_SECTOR, i);
        memset(tlist, 0, sizeof(tlist));

        // First all the things to process.
        for(k = 0, iter = P_GetPtrp(sec, DMT_MOBJS);
            k < MAXLIST - 1 && iter; iter = iter->sNext)
        {
            if(iter->type == MT_KEYGIZMOBLUE ||
               iter->type == MT_KEYGIZMOGREEN ||
               iter->type == MT_KEYGIZMOYELLOW)
                tlist[k++] = iter;
        }

        // Turn to face away from the nearest door.
        for(t = 0; (iter = tlist[t]) != NULL; ++t)
        {
            closestline = NULL;
            for(l = 0; l < numlines; ++l)
            {
                float               d1[2];

                li = P_ToPtr(DMU_LINEDEF, l);

                if(P_GetPtrp(li, DMU_BACK_SECTOR))
                    continue;

                xli = P_ToXLine(li);

                // It must be a special line with a back sector.
                if((xli->special != 32 && xli->special != 33 &&
                    xli->special != 34 && xli->special != 26 &&
                    xli->special != 27 && xli->special != 28))
                    continue;

                P_GetFloatpv(li, DMU_DXY, d1);
                linelen = P_ApproxDistance(d1[0], d1[1]);

                dist = fabs(P_PointLineDistance(li, iter->pos[VX],
                                                iter->pos[VY], &off));
                if(!closestline || dist < closestdist)
                {
                    closestdist = dist;
                    closestline = li;
                }
            }

            if(closestline)
            {
                vertex_t*       v0, *v1;
                float           v0p[2], v1p[2];

                v0 = P_GetPtrp(closestline, DMU_VERTEX0);
                v1 = P_GetPtrp(closestline, DMU_VERTEX1);

                P_GetFloatpv(v0, DMU_XY, v0p);
                P_GetFloatpv(v1, DMU_XY, v1p);

                iter->angle = R_PointToAngle2(v0p[VX], v0p[VY],
                                              v1p[VX], v1p[VY]) - ANG90;
            }
        }
    }
}
#endif
