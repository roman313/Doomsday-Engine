/**
 * @file bsp_intersection.c
 * BSP Builder Intersections. @ingroup map
 *
 * Based on glBSP 2.24 (in turn, based on BSP 2.3), which is hosted on
 * SourceForge: http://sourceforge.net/projects/glbsp/
 *
 * @authors Copyright © 2007-2012 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 2000-2007 Andrew Apted <ajapted@gmail.com>
 * @authors Copyright © 1998-2000 Colin Reed <cph@moria.org.uk>
 * @authors Copyright © 1998-2000 Lee Killough <killough@rsn.hp.com>
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

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#include "de_base.h"
#include "de_console.h"
#include "de_bsp.h"

#include "m_misc.h"
#include "edit_map.h"

#include "bspbuilder/intersection.hh"
#include "bspbuilder/superblockmap.hh"
#include "bspbuilder/bspbuilder.hh"

using namespace de;

struct hplaneintercept_s {
    struct hplaneintercept_s* next;
    struct hplaneintercept_s* prev;

    // How far along the partition line the vertex is. Zero is at the
    // partition half-edge's start point, positive values move in the same
    // direction as the partition's direction, and negative values move
    // in the opposite direction.
    double distance;

    void* userData;
};

HPlaneIntercept* HPlaneIntercept_Next(HPlaneIntercept* bi)
{
    assert(bi);
    return bi->next;
}

HPlaneIntercept* HPlaneIntercept_Prev(HPlaneIntercept* bi)
{
    assert(bi);
    return bi->prev;
}

void* HPlaneIntercept_UserData(HPlaneIntercept* bi)
{
    assert(bi);
    return bi->userData;
}

struct hplane_s {
    double origin[2];
    double angle[2];

    /// The intercept list. Kept sorted by along_dist, in ascending order.
    HPlaneIntercept* headPtr;

    /// Additional information used by the node builder during construction.
    HPlaneBuildInfo info;
};

static boolean initedOK = false;
static HPlaneIntercept* usedIntercepts;

static HPlaneIntercept* newIntercept(void)
{
    HPlaneIntercept* node;

    if(initedOK && usedIntercepts)
    {
        node = usedIntercepts;
        usedIntercepts = usedIntercepts->next;
    }
    else
    {
        // Need to allocate another.
        node = (HPlaneIntercept*)M_Malloc(sizeof *node);
    }

    node->userData = NULL;
    node->next = node->prev = NULL;

    return node;
}

HPlane* HPlane_New(void)
{
    HPlane* bi = (HPlane*)M_Malloc(sizeof *bi);
    bi->headPtr = NULL;
    return bi;
}

void HPlane_Delete(HPlane* bi, de::BspBuilder* builder)
{
    assert(bi);
    HPlane_Clear(bi, builder);
    M_Free(bi);
}

void HPlane_Clear(HPlane* bi, de::BspBuilder* builder)
{
    HPlaneIntercept* node;
    assert(bi);

    node = bi->headPtr;
    while(node)
    {
        HPlaneIntercept* p = node->next;

        builder->deleteHEdgeIntercept((HEdgeIntercept*)node->userData);

        // Move the bi node to the unused node bi.
        node->next = usedIntercepts;
        usedIntercepts = node;

        node = p;
    }

    bi->headPtr = NULL;
}

const double* HPlane_Origin(HPlane* bi)
{
    assert(bi);
    return bi->origin;
}

double HPlane_X(HPlane* bi)
{
    assert(bi);
    return bi->origin[0];
}

double HPlane_Y(HPlane* bi)
{
    assert(bi);
    return bi->origin[1];
}

HPlane* HPlane_SetOrigin(HPlane* bi, double const origin[2], de::BspBuilder* builder)
{
    assert(bi);
    if(origin)
    {
        bi->origin[0] = origin[0];
        bi->origin[1] = origin[1];
        HPlane_Clear(bi, builder);
    }
    return bi;
}

HPlane* HPlane_SetXY(HPlane* bi, double x, double y, de::BspBuilder* builder)
{
    double origin[2];
    origin[0] = x;
    origin[1] = y;
    return HPlane_SetOrigin(bi, origin, builder);
}

HPlane* HPlane_SetX(HPlane* bi, double x, de::BspBuilder* builder)
{
    assert(bi);
    bi->origin[0] = x;
    HPlane_Clear(bi, builder);
    return bi;
}

HPlane* HPlane_SetY(HPlane* bi, double y, de::BspBuilder* builder)
{
    assert(bi);
    bi->origin[1] = y;
    HPlane_Clear(bi, builder);
    return bi;
}

const double* HPlane_Angle(HPlane* bi)
{
    assert(bi);
    return bi->angle;
}

double HPlane_DX(HPlane* bi)
{
    assert(bi);
    return bi->angle[0];
}

double HPlane_DY(HPlane* bi)
{
    assert(bi);
    return bi->angle[1];
}

HPlane* HPlane_SetAngle(HPlane* bi, double const angle[2], de::BspBuilder* builder)
{
    assert(bi);
    if(angle)
    {
        bi->angle[0] = angle[0];
        bi->angle[1] = angle[1];
        HPlane_Clear(bi, builder);
    }
    return bi;
}

HPlane* HPlane_SetDXY(HPlane* bi, double x, double y, de::BspBuilder* builder)
{
    double angle[2];
    angle[0] = x;
    angle[1] = y;
    return HPlane_SetAngle(bi, angle, builder);
}

HPlane* HPlane_SetDX(HPlane* bi, double dx, de::BspBuilder* builder)
{
    assert(bi);
    bi->angle[0] = dx;
    HPlane_Clear(bi, builder);
    return bi;
}

HPlane* HPlane_SetDY(HPlane* bi, double dy, de::BspBuilder* builder)
{
    assert(bi);
    bi->angle[1] = dy;
    HPlane_Clear(bi, builder);
    return bi;
}

HPlaneBuildInfo* HPlane_BuildInfo(HPlane* bi)
{
    assert(bi);
    return &bi->info;
}

int HPlane_IterateIntercepts2(HPlane* bi, int (*callback)(HPlaneIntercept*, void*), void* parameters)
{
    assert(bi);
    if(callback)
    {
        HPlaneIntercept* node;
        for(node = bi->headPtr; node; node = node->next)
        {
            int result = callback(node, parameters);
            if(result) return result; // Stop iteration.
        }
    }
    return false; // Continue iteration.
}

int HPlane_IterateIntercepts(HPlane* bi, int (*callback)(HPlaneIntercept*, void*))
{
    return HPlane_IterateIntercepts2(bi, callback, NULL/*no parameters*/);
}

void Bsp_MergeHEdgeIntercepts(HEdgeIntercept* final, const HEdgeIntercept* other)
{
    if(!final || !other)
        Con_Error("Bsp_MergeIntersections2: Invalid arguments");

/*  DEBUG_Message((" Merging intersections:\n"));
#if _DEBUG
    Bsp_PrintHEdgeIntercept(final);
    Bsp_PrintHEdgeIntercept(other);
#endif*/

    if(final->selfRef && !other->selfRef)
    {
        if(final->before && other->before)
            final->before = other->before;

        if(final->after && other->after)
            final->after = other->after;

        final->selfRef = false;
    }

    if(!final->before && other->before)
        final->before = other->before;

    if(!final->after && other->after)
        final->after = other->after;

/*  DEBUG_Message((" Result:\n"));
#if _DEBUG
    Bsp_PrintHEdgeIntercept(final);
#endif*/
}

void BspBuilder::mergeIntersections(HPlane* hPlane)
{
    HPlaneIntercept* node, *np;

    if(!hPlane) return;

    node = hPlane->headPtr;
    np = node->next;
    while(node && np)
    {
        HEdgeIntercept* cur = (HEdgeIntercept*)node->userData;
        HEdgeIntercept* next = (HEdgeIntercept*)np->userData;
        double len = np->distance - node->distance;

        if(len < -0.1)
        {
            Con_Error("BspBuilder_MergeIntersections: Invalid intercept order - %1.3f > %1.3f\n",
                      node->distance, np->distance);
        }
        else if(len > 0.2)
        {
            node = np;
            np = node->next;
            continue;
        }
        /*else if(len > DIST_EPSILON)
        {
            DEBUG_Message((" Skipping very short half-edge (len=%1.3f) near (%1.1f,%1.1f)\n",
                           len, cur->vertex->V_pos[VX], cur->vertex->V_pos[VY]));
        }*/

        // Unlink this intercept.
        node->next = np->next;

        // Merge info for the two intersections into one.
        Bsp_MergeHEdgeIntercepts(cur, next);

        // Destroy the orphaned info.
        deleteHEdgeIntercept(next);

        np = node->next;
    }
}

void BspBuilder::buildHEdgesAtIntersectionGaps(HPlane* hPlane, SuperBlock* rightList,
    SuperBlock* leftList)
{
    HPlaneIntercept* node;

    if(!hPlane) return;

    node = hPlane->headPtr;
    while(node && node->next)
    {
        HEdgeIntercept* cur = (HEdgeIntercept*)node->userData;
        HEdgeIntercept* next = (HEdgeIntercept*)(node->next? node->next->userData : NULL);

        if(!(!cur->after && !next->before))
        {
            // Check for some nasty open/closed or close/open cases.
            if(cur->after && !next->before)
            {
                if(!cur->selfRef)
                {
                    double pos[2];

                    pos[VX] = cur->vertex->buildData.pos[VX] + next->vertex->buildData.pos[VX];
                    pos[VY] = cur->vertex->buildData.pos[VY] + next->vertex->buildData.pos[VY];

                    pos[VX] /= 2;
                    pos[VY] /= 2;

                    MPE_RegisterUnclosedSectorNear(cur->after, pos[VX], pos[VY]);
                }
            }
            else if(!cur->after && next->before)
            {
                if(!next->selfRef)
                {
                    double pos[2];

                    pos[VX] = cur->vertex->buildData.pos[VX] + next->vertex->buildData.pos[VX];
                    pos[VY] = cur->vertex->buildData.pos[VY] + next->vertex->buildData.pos[VY];
                    pos[VX] /= 2;
                    pos[VY] /= 2;

                    MPE_RegisterUnclosedSectorNear(next->before, pos[VX], pos[VY]);
                }
            }
            else
            {
                // This is definitetly open space.
                bsp_hedge_t* right, *left;

                // Do a sanity check on the sectors (just for good measure).
                if(cur->after != next->before)
                {
                    if(!cur->selfRef && !next->selfRef)
                    {
                        VERBOSE(
                        Con_Message("Sector mismatch: #%d (%1.1f,%1.1f) != #%d (%1.1f,%1.1f)\n",
                                    cur->after->buildData.index, cur->vertex->buildData.pos[VX],
                                    cur->vertex->buildData.pos[VY], next->before->buildData.index,
                                    next->vertex->buildData.pos[VX], next->vertex->buildData.pos[VY]));
                    }

                    // Choose the non-self-referencing sector when we can.
                    if(cur->selfRef && !next->selfRef)
                    {
                        cur->after = next->before;
                    }
                }

                addHEdgesBetweenIntercepts(hPlane, cur, next, &right, &left);

                // Add the new half-edges to the appropriate lists.
                SuperBlock_HEdgePush(rightList, right);
                SuperBlock_HEdgePush(leftList, left);
            }
        }

        node = node->next;
    }
}

HPlaneIntercept* HPlane_NewIntercept2(HPlane* bi, double distance, void* userData)
{
    HPlaneIntercept* after, *newNode;
    assert(bi);

    /**
     * Enqueue the new intercept into the bi.
     */
    after = bi->headPtr;
    while(after && after->next)
        after = after->next;

    while(after && distance < after->distance)
        after = after->prev;

    newNode = newIntercept();
    newNode->distance = distance;
    newNode->userData = userData;

    // Link it in.
    newNode->next = (after? after->next : bi->headPtr);
    newNode->prev = after;

    if(after)
    {
        if(after->next)
            after->next->prev = newNode;

        after->next = newNode;
    }
    else
    {
        if(bi->headPtr)
            bi->headPtr->prev = newNode;

        bi->headPtr = newNode;
    }

    return newNode;
}

HPlaneIntercept* HPlane_NewIntercept(HPlane* bi, double distance)
{
    return HPlane_NewIntercept2(bi, distance, NULL/*no user data*/);
}

#if _DEBUG
void HPlane_Print(HPlane* bi)
{
    if(bi)
    {
        HPlaneIntercept* node;

        Con_Message("HPlane %p:\n", bi);
        node = bi->headPtr;
        while(node)
        {
            HEdgeIntercept* inter = (HEdgeIntercept*)node->userData;
            Con_Printf(" %i: >%1.2f ", node->distance);
            Bsp_PrintHEdgeIntercept(inter);
            node = node->next;
        }
    }
}
#endif

void BspBuilder::initHPlaneInterceptAllocator(void)
{
    if(!initedOK)
    {
        usedIntercepts = NULL;
        initedOK = true;
    }
}

void BspBuilder::shutdownHPlaneInterceptAllocator(void)
{
    if(usedIntercepts)
    {
        HPlaneIntercept* node;

        node = usedIntercepts;
        while(node)
        {
            HPlaneIntercept* np = node->next;

            M_Free(node);
            node = np;
        }

        usedIntercepts = NULL;
    }

    initedOK = false;
}