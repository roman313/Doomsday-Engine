/** @file p_polyobjs.cpp
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2006-2013 Daniel Swanson <danij@dengine.net>
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
 * General Public License along with this program; if not, see:
 * http://www.gnu.org/licenses</small>
 */

#define DENG_NO_API_MACROS_MAP

#include "de_base.h"
#include "de_play.h"

// Called when the polyobj hits a mobj.
static void (*po_callback) (mobj_t* mobj, void* line, void* polyobj);

void P_PolyobjCallback(mobj_t* mobj, LineDef* lineDef, Polyobj* polyobj)
{
    if(!po_callback) return;
    po_callback(mobj, lineDef, polyobj);
}

DENG_EXTERN_C void P_SetPolyobjCallback(void (*func) (struct mobj_s*, void*, void*))
{
    po_callback = func;
}

void P_PolyobjChanged(Polyobj* po)
{
    LineDef** lineIter;
    for(lineIter = po->lines; *lineIter; lineIter++)
    {
        LineDef* line = *lineIter;
        HEdge* hedge = line->L_frontside.hedgeLeft;
        int i;

        // Shadow bias must be told.
        for(i = 0; i < 3; ++i)
        {
            SB_SurfaceMoved(hedge->bsuf[i]);
        }
    }
}

DENG_EXTERN_C void P_PolyobjUnlink(Polyobj* po)
{
    GameMap* map = theMap; /// @todo Do not assume polyobj is from the CURRENT map.
    GameMap_UnlinkPolyobj(map, po);
}

DENG_EXTERN_C void P_PolyobjLink(Polyobj* po)
{
    GameMap* map = theMap; /// @todo Do not assume polyobj is from the CURRENT map.
    GameMap_LinkPolyobj(map, po);
}

/// @note Part of the Doomsday public API
DENG_EXTERN_C Polyobj* P_PolyobjByID(uint id)
{
    if(!theMap) return NULL;
    return GameMap_PolyobjByID(theMap, id);
}

/// @note Part of the Doomsday public API
DENG_EXTERN_C Polyobj* P_PolyobjByTag(int tag)
{
    if(!theMap) return NULL;
    return GameMap_PolyobjByTag(theMap, tag);
}

/// @note Part of the Doomsday public API
DENG_EXTERN_C Polyobj* P_PolyobjByBase(void* ddMobjBase)
{
    if(!theMap) return NULL;
    return GameMap_PolyobjByBase(theMap, ddMobjBase);
}

/// @note Part of the Doomsday public API
DENG_EXTERN_C boolean P_PolyobjMove(Polyobj* po, coord_t xy[2])
{
    if(!po) return false;
    return Polyobj_Move(po, xy);
}

/// @note Part of the Doomsday public API
DENG_EXTERN_C boolean P_PolyobjMoveXY(Polyobj* po, coord_t x, coord_t y)
{
    if(!po) return false;
    return Polyobj_MoveXY(po, x, y);
}

/// @note Part of the Doomsday public API
DENG_EXTERN_C boolean P_PolyobjRotate(Polyobj* po, angle_t angle)
{
    if(!po) return false;
    return Polyobj_Rotate(po, angle);
}