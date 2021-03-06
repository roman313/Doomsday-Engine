/**
 * @file p_actor.c
 * Common code relating to mobj management. @ingroup libcommon
 *
 * @author Copyright &copy; 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @author Copyright &copy; 2006-2013 Daniel Swanson <danij@dengine.net>
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

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "p_tick.h"
#include "p_actor.h"

#define MIN_STEP                ((10 * ANGLE_1) >> 16) ///< Degrees per tic
#define MAX_STEP                (ANG90 >> 16)

#if __JDOOM64__
# define RESPAWNTICS            (4 * TICSPERSEC)
#else
# define RESPAWNTICS            (30 * TICSPERSEC)
#endif

typedef struct spawnqueuenode_s {
    int startTime;
    int minTics; ///< Minimum number of tics before respawn.
    void (*callback) (mobj_t* mo, void* context);
    void* context;

    coord_t pos[3];
    angle_t angle;
    mobjtype_t type;
    int spawnFlags; ///< MSF_* flags

    struct spawnqueuenode_s* next;
} spawnqueuenode_t;

static spawnqueuenode_t* spawnQueueHead = NULL, *unusedNodes = NULL;

void P_SpawnTelefog(mobj_t* mo, void* context)
{
#if __JDOOM__ || __JDOOM64__
    S_StartSound(SFX_ITMBK, mo);
#else
    S_StartSound(SFX_RESPAWN, mo);
#endif

# if __JDOOM64__
    mo->translucency = 255;
    mo->spawnFadeTics = 0;
    mo->intFlags |= MIF_FADE;
# elif __JDOOM__
    // Spawn the item teleport fog at the new spot.
    P_SpawnMobj(MT_IFOG, mo->origin, mo->angle, 0);
# endif
}

void P_MobjRemove(mobj_t* mo, boolean noRespawn)
{
    if(mo->ddFlags & DDMF_REMOTE) goto justDoIt;

#if __JDOOM__ || __JDOOM64__
    if(!noRespawn)
    {
        if(
# if __JDOOM__
            // Only respawn items in deathmatch 2 and optionally in coop.
           !(deathmatch != 2 && (!cfg.coopRespawnItems || !IS_NETGAME ||
                                 deathmatch)) &&
# endif /*#elif __JDOOM64__
           (spot->flags & MTF_RESPAWN) &&
# endif*/
           (mo->flags & MF_SPECIAL) && !(mo->flags & MF_DROPPED)
# if __JDOOM__ || __JDOOM64__
           && (mo->type != MT_INV) && (mo->type != MT_INS)
# endif
           )
        {
            P_DeferSpawnMobj3fv(RESPAWNTICS, mo->type, mo->spawnSpot.origin,
                                mo->spawnSpot.angle, mo->spawnSpot.flags,
                                P_SpawnTelefog, NULL);
        }
    }
#endif

#if __JHEXEN__
    if((mo->flags & MF_COUNTKILL) && (mo->flags & MF_CORPSE))
    {
        P_RemoveCorpseInQueue(mo);
    }

    P_MobjRemoveFromTIDList(mo);
#endif

justDoIt:
    P_MobjDestroy(mo);
}

void P_RemoveAllPlayerMobjs(void)
{
    uint i;
    for(i = 0; i < MAXPLAYERS; ++i)
    {
        player_t* plr = players + i;
        ddplayer_t* ddplr = plr->plr;
        if(!ddplr->inGame) continue;

        P_MobjRemove(ddplr->mo, true);
    }
}

void P_MobjSetOrigin(mobj_t* mo)
{
    int flags = DDLINK_BLOCKMAP; // Always.

    if(!(mo->flags & MF_NOSECTOR))
        flags |= DDLINK_SECTOR;

    P_MobjLink(mo, flags);
}

void P_MobjUnsetOrigin(mobj_t* mo)
{
    P_MobjUnlink(mo);
}

void P_MobjSetSRVO(mobj_t* mo, coord_t stepx, coord_t stepy)
{
    mo->srvo[VX] = -stepx;
    mo->srvo[VY] = -stepy;
}

void P_MobjSetSRVOZ(mobj_t* mo, coord_t stepz)
{
    mo->srvo[VZ] = -stepz;
}

void P_MobjAngleSRVOTicker(mobj_t* mo)
{
    short target, step, diff;
    int lstep, hgt;

    // Check requirements.
    if(mo->flags & MF_MISSILE || !(mo->flags & MF_COUNTKILL))
    {
        mo->visAngle = mo->angle >> 16;
        return; // This is not for us.
    }

    target = mo->angle >> 16;
    diff = target - mo->visAngle;

    if(mo->turnTime)
    {
        if(mo->tics) step = abs(diff) / mo->tics;
        else         step = abs(diff);

        if(!step) step = 1;
    }
    else
    {
        // Calculate a good step size.
        // Thing height and diff taken into account.
        hgt = (int) mo->height;
        hgt = MINMAX_OF(30, hgt, 60);

        lstep = abs(diff) * 8 / hgt;
        lstep = MINMAX_OF(MIN_STEP, lstep, MAX_STEP);

        step = lstep;
    }

    // Do the step.
    if(abs(diff) <= step)
        mo->visAngle  = target;
    else if(diff > 0)
        mo->visAngle += step;
    else if(diff < 0)
        mo->visAngle -= step;
}

void P_MobjClearSRVO(mobj_t* mo)
{
    memset(mo->srvo, 0, sizeof(mo->srvo));
}

boolean P_MobjIsCamera(const mobj_t* mo)
{
    // Client mobjs do not have thinkers and thus cannot be cameras.
    return (mo && mo->thinker.function && mo->player &&
            (mo->player->plr->flags & DDPF_CAMERA));
}

void P_UpdateHealthBits(mobj_t* mo)
{
    int i;
    if(!mo || !mo->info) return;

    if(mo->info->spawnHealth > 0)
    {
        mo->selector &= DDMOBJ_SELECTOR_MASK; // Clear high byte.

        i = (mo->health << 3) / mo->info->spawnHealth;
        i = MINMAX_OF(0, i, 7);

        mo->selector |= i << DDMOBJ_SELECTOR_SHIFT;
    }
}

statenum_t P_GetState(mobjtype_t type, statename_t name)
{
    if(type < MT_FIRST || type >= Get(DD_NUMMOBJTYPES)) return S_NULL;
    if(name < 0 || name >= STATENAMES_COUNT) return S_NULL;

    return MOBJINFO[type].states[name];
}

void P_RipperBlood(mobj_t* actor)
{
    mobj_t* mo;
    coord_t pos[3];

    pos[VX] = actor->origin[VX];
    pos[VY] = actor->origin[VY];
    pos[VZ] = actor->origin[VZ];

    pos[VX] += FIX2FLT((P_Random() - P_Random()) << 12);
    pos[VY] += FIX2FLT((P_Random() - P_Random()) << 12);
    pos[VZ] += FIX2FLT((P_Random() - P_Random()) << 12);

    if((mo = P_SpawnMobj(MT_BLOOD, pos, actor->angle, 0)))
    {
#if __JHERETIC__
        mo->flags |= MF_NOGRAVITY;
#endif
        mo->mom[MX] = actor->mom[MX] / 2;
        mo->mom[MY] = actor->mom[MY] / 2;
        mo->tics += P_Random() & 3;
    }
}

static spawnqueuenode_t* allocateNode(void)
{
#define SPAWNQUEUENODE_BATCHSIZE 32

    spawnqueuenode_t* n;

    if(unusedNodes)
    {
        // There are existing nodes we can re-use.
        n = unusedNodes;
        unusedNodes = unusedNodes->next;
        n->next = NULL;
    }
    else
    {
        // We need to allocate more.
        size_t i;
        spawnqueuenode_t* storage = Z_Malloc(sizeof(*n) * SPAWNQUEUENODE_BATCHSIZE, PU_GAMESTATIC, 0);

        // Add all but one to the unused node list.
        for(i = 0; i < SPAWNQUEUENODE_BATCHSIZE-1; ++i)
        {
            n = storage++;
            n->next = unusedNodes;
            unusedNodes = n;
        }

        n = storage;
    }

    return n;

#undef SPAWNQUEUENODE_BATCHSIZE
}

static void freeNode(spawnqueuenode_t* node, boolean recycle)
{
    // Find this node in the spawn queue and unlink it if found.
    if(spawnQueueHead)
    {
        if(spawnQueueHead == node)
        {
            spawnQueueHead = spawnQueueHead->next;
        }
        else
        {
            spawnqueuenode_t* n;
            for(n = spawnQueueHead; n->next; n = n->next)
            {
                if(n->next == node)
                    n->next = n->next->next;
            }
        }
    }

    if(recycle)
    {
        // Recycle this node for later use.
        node->next = unusedNodes;
        unusedNodes = node;
        return;
    }

    Z_Free(node);
}

static spawnqueuenode_t* dequeueSpawn(void)
{
    spawnqueuenode_t* n = spawnQueueHead;
    if(spawnQueueHead)
        spawnQueueHead = spawnQueueHead->next;
    return n;
}

static void emptySpawnQueue(boolean recycle)
{
    if(spawnQueueHead)
    {
        spawnqueuenode_t* n;
        while((n = dequeueSpawn()))
        {
            freeNode(n, recycle);
        }
    }
    spawnQueueHead = NULL;
}

static void enqueueSpawn(int minTics, mobjtype_t type, coord_t x, coord_t y,
    coord_t z, angle_t angle, int spawnFlags,
    void (*callback) (mobj_t* mo, void* context), void* context)
{
    spawnqueuenode_t* n = allocateNode();

    n->type = type;
    n->pos[VX] = x;
    n->pos[VY] = y;
    n->pos[VZ] = z;
    n->angle = angle;
    n->spawnFlags = spawnFlags;

    n->startTime = mapTime;
    n->minTics = minTics;

    n->callback = callback;
    n->context = context;

    if(spawnQueueHead)
    {
        // Find the correct insertion point.
        if(spawnQueueHead->next)
        {
            spawnqueuenode_t* l = spawnQueueHead;

            while(l->next && l->next->minTics - (mapTime - l->next->startTime) <= minTics)
            {
                l = l->next;
            }

            n->next = (l->next? l->next : NULL);
            l->next = n;
        }
        else
        {
            // After or before the head?
            if(spawnQueueHead->minTics - (mapTime - spawnQueueHead->startTime) <= minTics)
            {
                n->next = NULL;
                spawnQueueHead->next = n;
            }
            else
            {
                n->next = spawnQueueHead;
                spawnQueueHead = n;
            }
        }
    }
    else
    {
        n->next = NULL;
        spawnQueueHead = n;
    }
}

static mobj_t* doDeferredSpawn(void)
{
    mobj_t* mo = NULL;

    // Anything due to spawn?
    if(spawnQueueHead &&
       mapTime - spawnQueueHead->startTime >= spawnQueueHead->minTics)
    {
        spawnqueuenode_t* n = dequeueSpawn();

        // Spawn it.
        if((mo = P_SpawnMobj(n->type, n->pos, n->angle, n->spawnFlags)))
        {
            if(n->callback)
                n->callback(mo, n->context);
        }

        freeNode(n, true);
    }

    return mo;
}

void P_DeferSpawnMobj3f(int minTics, mobjtype_t type, coord_t x, coord_t y, coord_t z,
    angle_t angle, int spawnFlags,  void (*callback) (mobj_t* mo, void* context),
    void* context)
{
    if(minTics > 0)
    {
        enqueueSpawn(minTics, type, x, y, z, angle, spawnFlags, callback,
                     context);
    }
    else // Spawn immediately.
    {
        mobj_t* mo;
        if((mo = P_SpawnMobjXYZ(type, x, y, z, angle, spawnFlags)))
        {
            if(callback)
                callback(mo, context);
        }
    }
}

void P_DeferSpawnMobj3fv(int minTics, mobjtype_t type, coord_t const pos[3], angle_t angle,
    int spawnFlags, void (*callback) (mobj_t* mo, void* context), void* context)
{
    if(minTics > 0)
    {
        enqueueSpawn(minTics, type, pos[VX], pos[VY], pos[VZ], angle,
                     spawnFlags, callback, context);
    }
    else // Spawn immediately.
    {
        mobj_t* mo;
        if((mo = P_SpawnMobj(type, pos, angle, spawnFlags)))
        {
            if(callback)
                callback(mo, context);
        }
    }
}

/// @note Called 35 times per second by P_DoTick.
void P_ProcessDeferredSpawns(void)
{
    while(doDeferredSpawn())
    {}
}

void P_PurgeDeferredSpawns(void)
{
    emptySpawnQueue(true);
}
