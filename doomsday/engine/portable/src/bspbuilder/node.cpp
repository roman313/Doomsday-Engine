/**
 * @file bsp_node.c
 * BSP Builder Node. Recursive node creation and sorting. @ingroup map
 *
 * Based on glBSP 2.24 (in turn, based on BSP 2.3), which is hosted on
 * SourceForge: http://sourceforge.net/projects/glbsp/
 *
 * @authors Copyright © 2006-2012 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 2006-2007 Jamie Jones <jamie_jones_au@yahoo.com.au>
 * @authors Copyright © 2000-2007 Andrew Apted <ajapted@gmail.com>
 * @authors Copyright © 1998-2000 Colin Reed <cph@moria.org.uk>
 * @authors Copyright © 1998-2000 Lee Killough <killough@rsn.hp.com>
 * @authors Copyright © 1997-1998 Raphael.Quinet <raphael.quinet@eed.ericsson.se>
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

#include <math.h>

#include "de_base.h"
#include "de_console.h"
#include "de_bsp.h"
#include "de_play.h"
#include "de_misc.h"

#include "bspbuilder/intersection.hh"
#include "bspbuilder/superblockmap.hh"
#include "bspbuilder/bspbuilder.hh"

using namespace de;

typedef struct evalinfo_s {
    int cost;
    int splits;
    int iffy;
    int nearMiss;
    int realLeft;
    int realRight;
    int miniLeft;
    int miniRight;
} evalinfo_t;

// Used when sorting BSP leaf half-edges by angle around midpoint.
static size_t hedgeSortBufSize;
static bsp_hedge_t** hedgeSortBuf;

#if _DEBUG
void BSP_PrintSuperBlockhedges(SuperBlock* superblock);
#endif

static __inline int pointOnhedgeSide(double x, double y, const bsp_hedge_t* part)
{
    return P_PointOnLinedefSide2(x, y, part->info.pDX, part->info.pDY, part->info.pPerp,
                                 part->info.pLength, DIST_EPSILON);
}

static boolean getAveragedCoords(bsp_hedge_t* headPtr, double* x, double* y)
{
    size_t total = 0;
    double avg[2];
    bsp_hedge_t* cur;

    if(!x || !y) return false;

    avg[VX] = avg[VY] = 0;

    for(cur = headPtr; cur; cur = cur->nextInLeaf)
    {
        avg[VX] += cur->v[0]->buildData.pos[VX];
        avg[VY] += cur->v[0]->buildData.pos[VY];

        avg[VX] += cur->v[1]->buildData.pos[VX];
        avg[VY] += cur->v[1]->buildData.pos[VY];

        total += 2;
    }

    if(total > 0)
    {
        *x = avg[VX] / total;
        *y = avg[VY] / total;
        return true;
    }

    return false;
}

/**
 * Sort half-edges by angle (from the middle point to the start vertex).
 * The desired order (clockwise) means descending angles.
 *
 * @algorithm "double bubble"
 */
static void sorthedgesByAngleAroundPoint(bsp_hedge_t** hedges, size_t total,
    double x, double y)
{
    size_t i;

    i = 0;
    while(i + 1 < total)
    {
        bsp_hedge_t* a = hedges[i];
        bsp_hedge_t* b = hedges[i+1];
        double angle1, angle2;

        angle1 = M_SlopeToAngle(a->v[0]->buildData.pos[VX] - x,
                                a->v[0]->buildData.pos[VY] - y);
        angle2 = M_SlopeToAngle(b->v[0]->buildData.pos[VX] - x,
                                b->v[0]->buildData.pos[VY] - y);

        if(angle1 + ANG_EPSILON < angle2)
        {
            // Swap them.
            hedges[i] = b;
            hedges[i + 1] = a;

            // Bubble down.
            if(i > 0)
                i--;
        }
        else
        {
            // Bubble up.
            i++;
        }
    }
}

/**
 * Sort the given list of half-edges into clockwise order based on their
 * position/orientation compared to the specified point.
 *
 * @param headPtr       Ptr to the address of the headPtr to the list
 *                      of hedges to be sorted.
 * @param num           Number of half edges in the list.
 * @param x             X coordinate of the point to order around.
 * @param y             Y coordinate of the point to order around.
 */
static void clockwiseOrder(bsp_hedge_t** headPtr, size_t num, double x, double y)
{
    bsp_hedge_t* hedge;
    size_t i;

    // Insert ptrs to the hedges into the sort buffer.
    for(hedge = *headPtr, i = 0; hedge; hedge = hedge->nextInLeaf, ++i)
    {
        hedgeSortBuf[i] = hedge;
    }
    hedgeSortBuf[i] = NULL; // Terminate.

    if(i != num)
        Con_Error("clockwiseOrder: Miscounted?");

    sorthedgesByAngleAroundPoint(hedgeSortBuf, num, x, y);

    // Re-link the half-edge list in the order of the sorted array.
    *headPtr = NULL;
    for(i = 0; i < num; ++i)
    {
        size_t idx = (num - 1) - i;
        size_t j = idx % num;

        hedgeSortBuf[j]->nextInLeaf = *headPtr;
        *headPtr = hedgeSortBuf[j];
    }

/*#if _DEBUG
    Con_Message("Sorted half-edges around (%1.1f,%1.1f)\n", x, y);

    for(hedge = sub->hedges; hedge; hedge = hedge->next)
    {
        double angle = M_SlopeToAngle(hedge->v[0]->V_pos[VX] - x,
                                       hedge->v[0]->V_pos[VY] - y);

        Con_Message("  half-edge %p: Angle %1.6f  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
                    hedge, angle, hedge->v[0]->V_pos[VX], hedge->v[0]->V_pos[VY],
                    hedge->v[1]->V_pos[VX], hedge->v[1]->V_pos[VY]);
    }
#endif*/
}

static void sanityCheckClosed(const bspleafdata_t* leaf)
{
    int total = 0, gaps = 0;
    bsp_hedge_t* cur, *next;

    for(cur = leaf->hedges; cur; cur = cur->nextInLeaf)
    {
        next = (cur->nextInLeaf? cur->nextInLeaf : leaf->hedges);

        if(cur->v[1]->buildData.pos[VX] != next->v[0]->buildData.pos[VX] ||
           cur->v[1]->buildData.pos[VY] != next->v[0]->buildData.pos[VY])
        {
            gaps++;
        }

        total++;
    }

    if(gaps > 0)
    {
        VERBOSE( Con_Message("hedge list for leaf #%p is not closed (%d gaps, %d half-edges)\n", leaf, gaps, total) );

/*#if _DEBUG
    for(cur = leaf->hedges; cur; cur = cur->next)
    {
        Con_Message("  half-edge %p  (%1.1f,%1.1f) --> (%1.1f,%1.1f)\n", cur,
                    cur->v[0]->pos[VX], cur->v[0]->pos[VY],
                    cur->v[1]->pos[VX], cur->v[1]->pos[VY]);
    }
#endif*/
    }
}

static void sanityCheckSameSector(const bspleafdata_t* leaf)
{
    bsp_hedge_t* cur, *compare;

    // Find a suitable half-edge for comparison.
    for(compare = leaf->hedges; compare; compare = compare->nextInLeaf)
    {
        if(compare->sector) break;
    }

    if(!compare) return;

    for(cur = compare->nextInLeaf; cur; cur = cur->nextInLeaf)
    {
        if(!cur->sector) continue;

        if(cur->sector == compare->sector) continue;

        // Prevent excessive number of warnings.
        if(compare->sector->buildData.warnedFacing == cur->sector->buildData.index)
            continue;

        compare->sector->buildData.warnedFacing = cur->sector->buildData.index;

        if(verbose >= 1)
        {
            if(cur->info.lineDef)
                Con_Message("Sector #%d has sidedef facing #%d (line #%d).\n",
                            compare->sector->buildData.index, cur->sector->buildData.index,
                            cur->info.lineDef->buildData.index);
            else
                Con_Message("Sector #%d has sidedef facing #%d.\n",
                            compare->sector->buildData.index, cur->sector->buildData.index);
        }
    }
}

static boolean sanityCheckHasRealhedge(const bspleafdata_t* leaf)
{
    bsp_hedge_t* cur;
    for(cur = leaf->hedges; cur; cur = cur->nextInLeaf)
    {
        if(cur->info.lineDef) return true;
    }
    return false;
}

static void renumberLeafhedges(bspleafdata_t* leaf, uint* curIndex)
{
    uint n;
    bsp_hedge_t* cur;

    n = 0;
    for(cur = leaf->hedges; cur; cur = cur->nextInLeaf)
    {
        cur->index = *curIndex;
        (*curIndex)++;
        n++;
    }
}

static void preparehedgeSortBuffer(size_t numhedges)
{
    // Do we need to enlarge our sort buffer?
    if(numhedges + 1 > hedgeSortBufSize)
    {
        hedgeSortBufSize = numhedges + 1;
        hedgeSortBuf = (bsp_hedge_t**)M_Realloc(hedgeSortBuf, hedgeSortBufSize * sizeof(*hedgeSortBuf));
    }
}

static boolean C_DECL clockwiseLeaf(binarytree_t* tree, void* data)
{
    if(BinaryTree_IsLeaf(tree))
    {
        bspleafdata_t* leaf = (bspleafdata_t*) BinaryTree_GetData(tree);
        double midPoint[2] = { 0, 0 };
        bsp_hedge_t* hedge;
        size_t total;

        getAveragedCoords(leaf->hedges, &midPoint[VX], &midPoint[VY]);

        // Count half-edges.
        total = 0;
        for(hedge = leaf->hedges; hedge; hedge = hedge->nextInLeaf)
            total++;

        // Ensure the sort buffer is large enough.
        preparehedgeSortBuffer(total);

        clockwiseOrder(&leaf->hedges, total, midPoint[VX], midPoint[VY]);
        renumberLeafhedges(leaf, (uint*)data);

        // Do some sanity checks.
        sanityCheckClosed(leaf);
        sanityCheckSameSector(leaf);
        if(!sanityCheckHasRealhedge(leaf))
        {
            Con_Error("BSP Leaf #%p has no linedef-linked half-edge!", leaf);
        }
    }

    return true; // Continue traversal.
}

void BspBuilder::windLeafs(binarytree_t* rootNode)
{
    uint curIndex;

    hedgeSortBufSize = 0;
    hedgeSortBuf = NULL;

    curIndex = 0;
    BinaryTree_PostOrder(rootNode, clockwiseLeaf, &curIndex);

    // Free temporary storage.
    if(hedgeSortBuf)
    {
        M_Free(hedgeSortBuf);
        hedgeSortBuf = NULL;
    }
}

static __inline bspleafdata_t* allocBSPLeaf(void)
{
    bspleafdata_t* leafData = (bspleafdata_t*)M_Malloc(sizeof *leafData);
    return leafData;
}

static __inline void freeBSPLeaf(bspleafdata_t* leaf)
{
    M_Free(leaf);
}

bspleafdata_t* BspBuilder::newLeaf(void)
{
    bspleafdata_t* leaf = allocBSPLeaf();

    leaf->hedges = NULL;

    return leaf;
}

void BspBuilder::deleteLeaf(bspleafdata_t* leaf)
{
    bsp_hedge_t* cur, *np;

    if(!leaf) return;

    cur = leaf->hedges;
    while(cur)
    {
        np = cur->nextInLeaf;
        deleteHEdge(cur);
        cur = np;
    }

    freeBSPLeaf(leaf);
}

typedef struct {
    const BspHEdgeInfo* partInfo;
    int bestCost;
    evalinfo_t* info;
} evalpartitionworkerparams_t;

static int evalPartitionWorker2(bsp_hedge_t* check, void* parameters)
{
#define ADD_LEFT()  \
      do {  \
        if (check->info.lineDef) p->info->realLeft += 1;  \
        else                     p->info->miniLeft += 1;  \
      } while (0)

#define ADD_RIGHT()  \
      do {  \
        if (check->info.lineDef) p->info->realRight += 1;  \
        else                     p->info->miniRight += 1;  \
      } while (0)

    evalpartitionworkerparams_t* p = (evalpartitionworkerparams_t*) parameters;
    const int factor = bspFactor;
    double qnty, a, b, fa, fb;
    assert(p);

    // Catch "bad half-edges" early on.
    if(p->info->cost > p->bestCost) return true; // Stop iteration.

    // Get state of lines' relation to each other.
    if(check->info.sourceLineDef == p->partInfo->sourceLineDef)
    {
        a = b = fa = fb = 0;
    }
    else
    {
        a = M_PerpDist(p->partInfo->pDX, p->partInfo->pDY,
                       p->partInfo->pPerp, p->partInfo->pLength,
                       check->info.pSX, check->info.pSY);
        b = M_PerpDist(p->partInfo->pDX, p->partInfo->pDY,
                       p->partInfo->pPerp, p->partInfo->pLength,
                       check->info.pEX, check->info.pEY);

        fa = fabs(a);
        fb = fabs(b);
    }

    // Check for being on the same line.
    if(fa <= DIST_EPSILON && fb <= DIST_EPSILON)
    {
        // This half-edge runs along the same line as the partition.
        // Check whether it goes in the same direction or the opposite.
        if(check->info.pDX * p->partInfo->pDX + check->info.pDY * p->partInfo->pDY < 0)
        {
            ADD_LEFT();
        }
        else
        {
            ADD_RIGHT();
        }

        return false; // Continue iteration.
    }

    // Check for right side.
    if(a > -DIST_EPSILON && b > -DIST_EPSILON)
    {
        ADD_RIGHT();

        // Check for a near miss.
        if((a >= IFFY_LEN && b >= IFFY_LEN) ||
           (a <= DIST_EPSILON && b >= IFFY_LEN) ||
           (b <= DIST_EPSILON && a >= IFFY_LEN))
        {
            return false; // Continue iteration.
        }

        p->info->nearMiss++;

        /**
         * Near misses are bad, since they have the potential to cause really short
         * minihedges to be created in future processing. Thus the closer the near
         * miss, the higher the cost.
         */

        if(a <= DIST_EPSILON || b <= DIST_EPSILON)
            qnty = IFFY_LEN / MAX_OF(a, b);
        else
            qnty = IFFY_LEN / MIN_OF(a, b);

        p->info->cost += (int) (100 * factor * (qnty * qnty - 1.0));
        return false; // Continue iteration.
    }

    // Check for left side.
    if(a < DIST_EPSILON && b < DIST_EPSILON)
    {
        ADD_LEFT();

        // Check for a near miss.
        if((a <= -IFFY_LEN && b <= -IFFY_LEN) ||
           (a >= -DIST_EPSILON && b <= -IFFY_LEN) ||
           (b >= -DIST_EPSILON && a <= -IFFY_LEN))
        {
            return false; // Continue iteration.
        }

        p->info->nearMiss++;

        // The closer the miss, the higher the cost (see note above).
        if(a >= -DIST_EPSILON || b >= -DIST_EPSILON)
            qnty = IFFY_LEN / -MIN_OF(a, b);
        else
            qnty = IFFY_LEN / -MAX_OF(a, b);

        p->info->cost += (int) (70 * factor * (qnty * qnty - 1.0));
        return false; // Continue iteration.
    }

    /**
     * When we reach here, we have a and b non-zero and opposite sign,
     * hence this half-edge will be split by the partition line.
     */

    p->info->splits++;
    p->info->cost += 100 * factor;

    /**
     * Check if the split point is very close to one end, which is quite an undesirable
     * situation (producing really short edges). This is perhaps _one_ source of those
     * darn slime trails. Hence the name "IFFY segs" and a rather hefty surcharge.
     */
    if(fa < IFFY_LEN || fb < IFFY_LEN)
    {
        p->info->iffy++;

        // The closer to the end, the higher the cost.
        qnty = IFFY_LEN / MIN_OF(fa, fb);
        p->info->cost += (int) (140 * factor * (qnty * qnty - 1.0));
    }
    return false; // Continue iteration.

#undef ADD_RIGHT
#undef ADD_LEFT
}

/**
 * @return  @c true= iff a "bad half-edge" was found early.
 */
static int evalPartitionWorker(SuperBlock* hedgeList, bsp_hedge_t* part,
    int bestCost, evalinfo_t* info)
{
    const BspHEdgeInfo* partInfo = &part->info;
    evalpartitionworkerparams_t parm;
    int result, num;

    /**
     * Test the whole block against the partition line to quickly handle all the
     * half-edges within it at once. Only when the partition line intercepts the
     * box do we need to go deeper into it.
     */
    num = P_BoxOnLineSide3(SuperBlock_Bounds(hedgeList), partInfo->pSX, partInfo->pSY,
                           partInfo->pDX, partInfo->pDY, partInfo->pPerp,
                           partInfo->pLength, DIST_EPSILON);

    if(num < 0)
    {
        // Left.
        info->realLeft += SuperBlock_RealHEdgeCount(hedgeList);
        info->miniLeft += SuperBlock_MiniHEdgeCount(hedgeList);

        return false;
    }
    else if(num > 0)
    {
        // Right.
        info->realRight += SuperBlock_RealHEdgeCount(hedgeList);
        info->miniRight += SuperBlock_MiniHEdgeCount(hedgeList);

        return false;
    }

    // Check partition against all half-edges.
    parm.partInfo = partInfo;
    parm.bestCost = bestCost;
    parm.info = info;
    result = SuperBlock_IterateHEdges2(hedgeList, evalPartitionWorker2, (void*)&parm);
    if(result) return true;

    // Handle sub-blocks recursively.
    for(num = 0; num < 2; ++num)
    {
        SuperBlock* child = SuperBlock_Child(hedgeList, num);
        if(!child) continue;

        if(evalPartitionWorker(child, part, bestCost, info))
            return true;
    }

    // No "bad half-edge" was found. Good.
    return false;
}

/**
 * Evaluate a partition and determine the cost, taking into account the number of
 * splits and the difference between left and right.
 *
 * To be able to divide the nodes down, evalPartition must decide which is the best
 * half-edge to use as a nodeline. It does this by selecting the line with least
 * splits and has least difference of hald-edges on either side of it.
 *
 * @return  The computed cost, or a negative value if the edge should be skipped.
 */
static int evalPartition(SuperBlock* hedgeList, bsp_hedge_t* part, int bestCost)
{
    evalinfo_t info;

    // Initialize info structure.
    info.cost   = 0;
    info.splits = 0;
    info.iffy   = 0;
    info.nearMiss  = 0;

    info.realLeft  = 0;
    info.realRight = 0;
    info.miniLeft  = 0;
    info.miniRight = 0;

    if(evalPartitionWorker(hedgeList, part, bestCost, &info)) return -1;

    // Make sure there is at least one real hedge on each side.
    if(!info.realLeft || !info.realRight)
    {
        //DEBUG_Message(("Eval : No real half-edges on %s%sside\n",
        //              (info.realLeft? "" : "left "), (info.realRight? "" : "right ")));
        return -1;
    }

    // Increase cost by the difference between left and right.
    info.cost += 100 * ABS(info.realLeft - info.realRight);

    // Allow minihedge counts to affect the outcome.
    info.cost += 50 * ABS(info.miniLeft - info.miniRight);

    // Another little twist, here we show a slight preference for partition
    // lines that lie either purely horizontally or purely vertically.
    if(!FEQUAL(part->info.pDX, 0) && !FEQUAL(part->info.pDY, 0))
        info.cost += 25;

    //DEBUG_Message(("Eval %p: splits=%d iffy=%d near=%d left=%d+%d right=%d+%d "
    //               "cost=%d.%02d\n", part, info.splits, info.iffy, info.nearMiss,
    //               info.realLeft, info.miniLeft, info.realRight, info.miniRight,
    //               info.cost / 100, info.cost % 100));

    return info.cost;
}

typedef struct {
    SuperBlock* hedgeList;
    bsp_hedge_t** best;
    int* bestCost;
} pickhedgeworkerparams_t;

/**
 * @return  @c true= cancelled.
 */
static int pickhedgeWorker2(bsp_hedge_t* part, void* parameters)
{
    pickhedgeworkerparams_t* p = (pickhedgeworkerparams_t*)parameters;
    LineDef* lineDef = part->info.lineDef;
    int cost;

    //DEBUG_Message(("pickhedgeWorker: %shedge %p sector=%d  (%1.1f,%1.1f) -> "
    //               "(%1.1f,%1.1f)\n", (lineDef? "" : "MINI"), part,
    //               (part->sector? part->sector->index : -1),
    //               part->v[0]->V_pos[VX], part->v[0]->V_pos[VY],
    //               part->v[1]->V_pos[VX], part->v[1]->V_pos[VY]));

    // Ignore minihedges as partition candidates.
    if(!lineDef) return false; // Continue iteration.

    // Only test half-edges from the same linedef once per round of
    // partition picking (they are collinear).
    if(lineDef->validCount == validCount) return false; // Continue iteration.
    lineDef->validCount = validCount;

    // Unsuitable or too costly?
    cost = evalPartition(p->hedgeList, part, *p->bestCost);
    if(cost < 0 || cost >= *p->bestCost) return false; // Continue iteration.

    // We have a new better choice.
    (*p->bestCost) = cost;

    // Remember which half-edge.
    (*p->best) = part;

    return false; // Continue iteration.
}

static int pickhedgeWorker(SuperBlock* partList, void* parameters)
{
    // Test each half-edge as a potential partition.
    return SuperBlock_IterateHEdges2(partList, pickhedgeWorker2, parameters);
}

boolean BspBuilder::choosePartition(SuperBlock* hedgeList, size_t /*depth*/, HPlane* partition)
{
    pickhedgeworkerparams_t parm;
    int bestCost = INT_MAX;
    bsp_hedge_t* best = NULL;

    //DEBUG_Message(("BspBuilder::choosePartition: Begun (depth %lu)\n", (unsigned long) depth));

    parm.hedgeList = hedgeList;
    parm.best = &best;
    parm.bestCost = &bestCost;

    validCount++;
    if(SuperBlock_Traverse2(hedgeList, pickhedgeWorker, (void*)&parm))
    {
        /// @kludge BspBuilder::buildNodes() will detect the cancellation.
        return false;
    }

    // Finished, return the best partition.
    if(best)
    {
        LineDef* lineDef = best->info.lineDef;

        // This must not be a "mini hedge".
        assert(lineDef);

        //DEBUG_Message(("BspBuilder::choosePartition: Best has score %d.%02d  (%1.1f,%1.1f) -> "
        //               "(%1.1f,%1.1f)\n", bestCost / 100, bestCost % 100,
        //               best->v[0]->V_pos[VX], best->v[0]->V_pos[VY],
        //               best->v[1]->V_pos[VX], best->v[1]->V_pos[VY]));

        // Reconfigure the partition for the next round of hedge sorting.
        partition->setXY(lineDef->L_v(best->side)->buildData.pos[VX],
                         lineDef->L_v(best->side)->buildData.pos[VY]);
        partition->setDXY(lineDef->L_v(best->side^1)->buildData.pos[VX] - lineDef->L_v(best->side)->buildData.pos[VX],
                          lineDef->L_v(best->side^1)->buildData.pos[VY] - lineDef->L_v(best->side)->buildData.pos[VY]);

        BspHEdgeInfo* info = partition->partitionHEdgeInfo();
        memcpy(info, &best->info, sizeof(*info));

        return true;
    }

    //DEBUG_Message(("BspBuilder::choosePartition: No best found!\n"));
    return false;
}

static boolean lineDefHasSelfRef(LineDef* lineDef)
{
    return !!(lineDef->buildData.mlFlags & MLF_SELFREF);
}

const HPlaneIntercept* BspBuilder::makeHPlaneIntersection(HPlane* hplane, bsp_hedge_t* hedge, int leftSide)
{
    HEdgeIntercept* hedgeIntercept;
    const BspHEdgeInfo* info;
    const HPlaneIntercept* inter;
    Vertex* vertex;
    double distance;
    assert(hplane && hedge);

    // Already present on this edge?
    vertex = hedge->v[leftSide?1:0];
    inter = hplaneInterceptByVertex(hplane, vertex);
    if(inter) return inter;

    info = hplane->partitionHEdgeInfo();
    distance = M_ParallelDist(info->pDX, info->pDY, info->pPara, info->pLength,
                              vertex->buildData.pos[VX], vertex->buildData.pos[VY]);

    hedgeIntercept = newHEdgeIntercept(vertex, info, (hedge->info.lineDef && lineDefHasSelfRef(hedge->info.lineDef)));
    return hplane->newIntercept(distance, hedgeIntercept);
}

const HPlaneIntercept* BspBuilder::makeIntersection(HPlane* hplane, bsp_hedge_t* hedge, int leftSide)
{
    if(!hplane || !hedge)
        Con_Error("BspBuilder::makeHPlaneIntersection: Invalid arguments.");
    return makeHPlaneIntersection(hplane, hedge, leftSide);
}

/**
 * Calculate the intersection location between the current half-edge and the partition.
 * Takes advantage of some common situations like horizontal and vertical lines to
 * choose a 'nicer' intersection point.
 */
static __inline void calcIntersection(BspHEdgeInfo* hedge, const BspHEdgeInfo* other,
    double perpC, double perpD, double* x, double* y)
{
    double ds;

    // Horizontal partition against vertical half-edge.
    if(other->pDY == 0 && hedge->pDX == 0)
    {
        *x = hedge->pSX;
        *y = other->pSY;
        return;
    }

    // Vertical partition against horizontal half-edge.
    if(other->pDX == 0 && hedge->pDY == 0)
    {
        *x = other->pSX;
        *y = hedge->pSY;
        return;
    }

    // 0 = start, 1 = end.
    ds = perpC / (perpC - perpD);

    if(hedge->pDX == 0)
        *x = hedge->pSX;
    else
        *x = hedge->pSX + (hedge->pDX * ds);

    if(hedge->pDY == 0)
        *y = hedge->pSY;
    else
        *y = hedge->pSY + (hedge->pDY * ds);
}

void BspBuilder::divideHEdge(bsp_hedge_t* hedge, HPlane* partition, SuperBlock* rightList,
    SuperBlock* leftList)
{
    const BspHEdgeInfo* info = partition->partitionHEdgeInfo();
    bsp_hedge_t* newhedge;
    double x, y;
    double a, b;

    // Get state of lines' relation to each other.
    a = M_PerpDist(info->pDX, info->pDY, info->pPerp, info->pLength, hedge->info.pSX, hedge->info.pSY);
    b = M_PerpDist(info->pDX, info->pDY, info->pPerp, info->pLength, hedge->info.pEX, hedge->info.pEY);

    if(hedge->info.sourceLineDef == info->sourceLineDef)
        a = b = 0;

    // Check for being on the same line.
    if(fabs(a) <= DIST_EPSILON && fabs(b) <= DIST_EPSILON)
    {
        makeIntersection(partition, hedge, RIGHT);
        makeIntersection(partition, hedge, LEFT);

        // This hedge runs along the same line as the partition. Check whether it goes in
        // the same direction or the opposite.
        if(hedge->info.pDX * info->pDX + hedge->info.pDY * info->pDY < 0)
        {
            SuperBlock_HEdgePush(leftList, hedge);
        }
        else
        {
            SuperBlock_HEdgePush(rightList, hedge);
        }

        return;
    }

    // Check for right side.
    if(a > -DIST_EPSILON && b > -DIST_EPSILON)
    {
        if(a < DIST_EPSILON)
            makeIntersection(partition, hedge, RIGHT);
        else if(b < DIST_EPSILON)
            makeIntersection(partition, hedge, LEFT);

        SuperBlock_HEdgePush(rightList, hedge);
        return;
    }

    // Check for left side.
    if(a < DIST_EPSILON && b < DIST_EPSILON)
    {
        if(a > -DIST_EPSILON)
            makeIntersection(partition, hedge, RIGHT);
        else if(b > -DIST_EPSILON)
            makeIntersection(partition, hedge, LEFT);

        SuperBlock_HEdgePush(leftList, hedge);
        return;
    }

    // When we reach here, we have a and b non-zero and opposite sign, hence this edge
    // will be split by the partition line.

    calcIntersection(&hedge->info, info, a, b, &x, &y);
    newhedge = splitHEdge(hedge, x, y);
    makeIntersection(partition, hedge, LEFT);

    if(a < 0)
    {
        SuperBlock_HEdgePush(leftList,  hedge);
        SuperBlock_HEdgePush(rightList, newhedge);
    }
    else
    {
        SuperBlock_HEdgePush(rightList, hedge);
        SuperBlock_HEdgePush(leftList,  newhedge);
    }
}

typedef struct {
    SuperBlock* rights;
    SuperBlock* lefts;
    HPlane* hplane;
    BspBuilder* builder;
} partitionhedgeworkerparams_t;

int C_DECL BspBuilder_PartitionHEdgeWorker(SuperBlock* superblock, void* parameters)
{
    partitionhedgeworkerparams_t* p = (partitionhedgeworkerparams_t*)parameters;
    bsp_hedge_t* hedge;
    assert(p);

    while((hedge = SuperBlock_HEdgePop(superblock)))
    {
        p->builder->divideHEdge(hedge, p->hplane, p->rights, p->lefts);
    }

    return false; // Continue iteration.
}

void BspBuilder::partitionHEdges(SuperBlock* hedgeList, SuperBlock* rights, SuperBlock* lefts,
    HPlane* hplane)
{
    partitionhedgeworkerparams_t parm;

    parm.rights = rights;
    parm.lefts = lefts;
    parm.hplane = hplane;
    parm.builder = this;
    SuperBlock_Traverse2(hedgeList, BspBuilder_PartitionHEdgeWorker, (void*)&parm);

    // Sanity checks...
    if(!SuperBlock_TotalHEdgeCount(rights))
        Con_Error("BspBuilder::partitionhedges: Separated half-edge has no right side.");

    if(!SuperBlock_TotalHEdgeCount(lefts))
        Con_Error("BspBuilder::partitionhedges: Separated half-edge has no left side.");

    addMiniHEdges(hplane, rights, lefts);
}

static int createBSPLeafWorker(SuperBlock* superblock, void* parameters)
{
    bspleafdata_t* leaf = (bspleafdata_t*) parameters;
    bsp_hedge_t* hedge;
    assert(leaf);

    while((hedge = SuperBlock_HEdgePop(superblock)))
    {
        // Link it into head of the leaf's list.
        hedge->nextInLeaf = leaf->hedges;
        leaf->hedges = hedge;
    }

    return false; // Continue iteration.
}

/**
 * Create a new leaf from a list of half-edges.
 */
bspleafdata_t* BspBuilder::createBSPLeaf(SuperBlock* hedgeList)
{
    bspleafdata_t* leaf = newLeaf();

    // Link the half-edges into the new leaf.
    SuperBlock_Traverse2(hedgeList, createBSPLeafWorker, leaf);

    return leaf;
}

boolean BspBuilder::buildNodes(SuperBlock* superblock, binarytree_t** parent, size_t depth,
    HPlane* hplane)
{
    SuperBlockmap* hedgeSet[2];
    binarytree_t* subTree;
    bspleafdata_t* leaf;
    BspNode* node;
    boolean builtOK = false;

    *parent = NULL;

    /*DEBUG_Message(("Build: Begun @ %lu\n", (unsigned long) depth));
#if _DEBUG
    BSP_PrintSuperBlockhedges(superblock);
#endif*/

    // Pick the next partition to use.
    if(!choosePartition(superblock, depth, hplane))
    {
        // No partition required, already convex.
        //DEBUG_Message(("BspBuilder::buildNodes: Convex.\n"));

        leaf = createBSPLeaf(superblock);
        *parent = BinaryTree_Create(leaf);
        return true;
    }

    //DEBUG_Message(("BspBuilder::buildNodes: Partition %p (%1.0f,%1.0f) -> (%1.0f,%1.0f).\n",
    //               best, best->v[0]->V_pos[VX], best->v[0]->V_pos[VY],
    //               best->v[1]->V_pos[VX], best->v[1]->V_pos[VY]));

    // Create left and right super blockmaps.
    /// @todo There should be no need to construct entirely independent
    ///       data structures to contain these hedge subsets.
    // Copy the bounding box of the edge list to the superblocks.
    hedgeSet[RIGHT] = SuperBlockmap_New(SuperBlock_Bounds(superblock));
    hedgeSet[LEFT]  = SuperBlockmap_New(SuperBlock_Bounds(superblock));

    // Divide the half-edges into two lists: left & right.
    partitionHEdges(superblock, SuperBlockmap_Root(hedgeSet[RIGHT]),
                                SuperBlockmap_Root(hedgeSet[LEFT]), hplane);
    hplane->clear();

    node = (BspNode*)M_Calloc(sizeof *node);
    *parent = BinaryTree_Create(node);

    SuperBlockmap_FindHEdgeBounds(hedgeSet[LEFT],  &node->aaBox[LEFT]);
    SuperBlockmap_FindHEdgeBounds(hedgeSet[RIGHT], &node->aaBox[RIGHT]);

    node->partition.x = hplane->x();
    node->partition.y = hplane->y();
    node->partition.dX = hplane->dX();
    node->partition.dY = hplane->dY();

    builtOK = buildNodes(SuperBlockmap_Root(hedgeSet[RIGHT]), &subTree, depth + 1, hplane);
    BinaryTree_SetChild(*parent, RIGHT, subTree);
    SuperBlockmap_Delete(hedgeSet[RIGHT]);

    if(builtOK)
    {
        builtOK = buildNodes(SuperBlockmap_Root(hedgeSet[LEFT]), &subTree, depth + 1, hplane);
        BinaryTree_SetChild(*parent, LEFT, subTree);
    }

    SuperBlockmap_Delete(hedgeSet[LEFT]);

    return builtOK;
}

#if _DEBUG
static int printSuperBlockhedgesWorker2(bsp_hedge_t* hedge, void* /*parameters*/)
{
    Con_Message("Build: %s %p sector=%d (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
                (hedge->info.lineDef? "NORM" : "MINI"), hedge,
                hedge->sector->buildData.index,
                hedge->v[0]->buildData.pos[VX], hedge->v[0]->buildData.pos[VY],
                hedge->v[1]->buildData.pos[VX], hedge->v[1]->buildData.pos[VY]);
    return false; // Continue iteration.
}

static int printSuperBlockhedgesWorker(SuperBlock* superblock, void* parameters)
{
    return SuperBlock_IterateHEdges2(superblock, printSuperBlockhedgesWorker2, parameters);
}

void BSP_PrintSuperBlockhedges(SuperBlock* superblock)
{
    if(!superblock) return;
    SuperBlock_Traverse(superblock, printSuperBlockhedgesWorker);
}
#endif