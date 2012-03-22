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

// Used when sorting BSP leaf half-edges by angle around midpoint.
static size_t hEdgeSortBufSize;
static bsp_hedge_t** hEdgeSortBuf;

static __inline int pointOnHEdgeSide(double x, double y, const bsp_hedge_t* part)
{
    return P_PointOnLinedefSide2(x, y, part->pDX, part->pDY, part->pPerp,
                                 part->pLength, DIST_EPSILON);
}

/**
 * Add the given half-edge to the specified list.
 */
void BSP_AddHEdgeToSuperBlock(superblock_t* block, bsp_hedge_t* hEdge)
{
#define SUPER_IS_LEAF(s)  \
    ((s)->bbox[BOXRIGHT] - (s)->bbox[BOXLEFT] <= 256 && \
     (s)->bbox[BOXTOP] - (s)->bbox[BOXBOTTOM] <= 256)

    for(;;)
    {
        int p1, p2;
        int child;
        int midPoint[2];
        superblock_t* sub;

        midPoint[VX] = (block->bbox[BOXLEFT]   + block->bbox[BOXRIGHT]) / 2;
        midPoint[VY] = (block->bbox[BOXBOTTOM] + block->bbox[BOXTOP])   / 2;

        // Update half-edge counts.
        if(hEdge->lineDef)
            block->realNum++;
        else
            block->miniNum++;

        if(SUPER_IS_LEAF(block))
        {
            // Block is a leaf -- no subdivision possible.
            BSP_LinkHEdgeToSuperBlock(block, hEdge);
            return;
        }

        if(block->bbox[BOXRIGHT] - block->bbox[BOXLEFT] >=
           block->bbox[BOXTOP]   - block->bbox[BOXBOTTOM])
        {
            // Block is wider than it is high, or square.
            p1 = hEdge->v[0]->buildData.pos[VX] >= midPoint[VX];
            p2 = hEdge->v[1]->buildData.pos[VX] >= midPoint[VX];
        }
        else
        {
            // Block is higher than it is wide.
            p1 = hEdge->v[0]->buildData.pos[VY] >= midPoint[VY];
            p2 = hEdge->v[1]->buildData.pos[VY] >= midPoint[VY];
        }

        if(p1 && p2)
        {
            child = 1;
        }
        else if(!p1 && !p2)
        {
            child = 0;
        }
        else
        {
            // Line crosses midpoint -- link it in and return.
            BSP_LinkHEdgeToSuperBlock(block, hEdge);
            return;
        }

        // The hedge lies in one half of this block. Create the block if it
        // doesn't already exist, and loop back to add the hedge.
        if(!block->subs[child])
        {
            block->subs[child] = sub = BSP_SuperBlockCreate();
            sub->parent = block;

            if(block->bbox[BOXRIGHT] - block->bbox[BOXLEFT] >=
               block->bbox[BOXTOP]   - block->bbox[BOXBOTTOM])
            {
                sub->bbox[BOXLEFT]   = (child? midPoint[VX] : block->bbox[BOXLEFT]);
                sub->bbox[BOXBOTTOM] = block->bbox[BOXBOTTOM];

                sub->bbox[BOXRIGHT]  = (child? block->bbox[BOXRIGHT] : midPoint[VX]);
                sub->bbox[BOXTOP]    = block->bbox[BOXTOP];
            }
            else
            {
                sub->bbox[BOXLEFT]   = block->bbox[BOXLEFT];
                sub->bbox[BOXBOTTOM] = (child? midPoint[VY] : block->bbox[BOXBOTTOM]);

                sub->bbox[BOXRIGHT]  = block->bbox[BOXRIGHT];
                sub->bbox[BOXTOP]    = (child? block->bbox[BOXTOP] : midPoint[VY]);
            }
        }

        block = block->subs[child];
    }

#undef SUPER_IS_LEAF
}

static boolean getAveragedCoords(bsp_hedge_t* headPtr, double* x, double* y)
{
    size_t total = 0;
    double avg[2];
    bsp_hedge_t* cur;

    if(!x || !y) return false;

    avg[VX] = avg[VY] = 0;

    for(cur = headPtr; cur; cur = cur->next)
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
static void sortHEdgesByAngleAroundPoint(bsp_hedge_t** hEdges, size_t total,
    double x, double y)
{
    size_t i;

    i = 0;
    while(i + 1 < total)
    {
        bsp_hedge_t* a = hEdges[i];
        bsp_hedge_t* b = hEdges[i+1];
        angle_g angle1, angle2;

        angle1 = M_SlopeToAngle(a->v[0]->buildData.pos[VX] - x,
                                a->v[0]->buildData.pos[VY] - y);
        angle2 = M_SlopeToAngle(b->v[0]->buildData.pos[VX] - x,
                                b->v[0]->buildData.pos[VY] - y);

        if(angle1 + ANG_EPSILON < angle2)
        {
            // Swap them.
            hEdges[i] = b;
            hEdges[i + 1] = a;

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
    bsp_hedge_t* hEdge;
    size_t i;

    // Insert ptrs to the hEdges into the sort buffer.
    for(hEdge = *headPtr, i = 0; hEdge; hEdge = hEdge->next, ++i)
    {
        hEdgeSortBuf[i] = hEdge;
    }
    hEdgeSortBuf[i] = NULL; // Terminate.

    if(i != num)
        Con_Error("clockwiseOrder: Miscounted?");

    sortHEdgesByAngleAroundPoint(hEdgeSortBuf, num, x, y);

    // Re-link the half-edge list in the order of the sorted array.
    *headPtr = NULL;
    for(i = 0; i < num; ++i)
    {
        size_t idx = (num - 1) - i;
        size_t j = idx % num;

        hEdgeSortBuf[j]->next = *headPtr;
        *headPtr = hEdgeSortBuf[j];
    }

/*#if _DEBUG
    Con_Message("Sorted half-edges around (%1.1f,%1.1f)\n", x, y);

    for(hEdge = sub->hEdges; hEdge; hEdge = hEdge->next)
    {
        angle_g angle = M_SlopeToAngle(hEdge->v[0]->V_pos[VX] - x,
                                       hEdge->v[0]->V_pos[VY] - y);

        Con_Message("  half-edge %p: Angle %1.6f  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
                    hEdge, angle, hEdge->v[0]->V_pos[VX], hEdge->v[0]->V_pos[VY],
                    hEdge->v[1]->V_pos[VX], hEdge->v[1]->V_pos[VY]);
    }
#endif*/
}

static void sanityCheckClosed(const bspleafdata_t* leaf)
{
    int total = 0, gaps = 0;
    bsp_hedge_t* cur, *next;

    for(cur = leaf->hEdges; cur; cur = cur->next)
    {
        next = (cur->next? cur->next : leaf->hEdges);

        if(cur->v[1]->buildData.pos[VX] != next->v[0]->buildData.pos[VX] ||
           cur->v[1]->buildData.pos[VY] != next->v[0]->buildData.pos[VY])
        {
            gaps++;
        }

        total++;
    }

    if(gaps > 0)
    {
        VERBOSE( Con_Message("HEdge list for leaf #%p is not closed (%d gaps, %d half-edges)\n", leaf, gaps, total) );

/*#if _DEBUG
    for(cur = leaf->hEdges; cur; cur = cur->next)
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
    for(compare = leaf->hEdges; compare; compare = compare->next)
    {
        if(compare->sector) break;
    }

    if(!compare) return;

    for(cur = compare->next; cur; cur = cur->next)
    {
        if(!cur->sector) continue;

        if(cur->sector == compare->sector) continue;

        // Prevent excessive number of warnings.
        if(compare->sector->buildData.warnedFacing == cur->sector->buildData.index)
            continue;

        compare->sector->buildData.warnedFacing = cur->sector->buildData.index;

        if(verbose >= 1)
        {
            if(cur->lineDef)
                Con_Message("Sector #%d has sidedef facing #%d (line #%d).\n", compare->sector->buildData.index,
                            cur->sector->buildData.index, cur->lineDef->buildData.index);
            else
                Con_Message("Sector #%d has sidedef facing #%d.\n",
                            compare->sector->buildData.index, cur->sector->buildData.index);
        }
    }
}

static boolean sanityCheckHasRealHEdge(const bspleafdata_t* leaf)
{
    bsp_hedge_t* cur;

    for(cur = leaf->hEdges; cur; cur = cur->next)
    {
        if(cur->lineDef)
            return true;
    }

    return false;
}

static void renumberLeafHEdges(bspleafdata_t* leaf, uint* curIndex)
{
    uint n;
    bsp_hedge_t* cur;

    n = 0;
    for(cur = leaf->hEdges; cur; cur = cur->next)
    {
        cur->index = *curIndex;
        (*curIndex)++;
        n++;
    }
}

static void prepareHEdgeSortBuffer(size_t numHEdges)
{
    // Do we need to enlarge our sort buffer?
    if(numHEdges + 1 > hEdgeSortBufSize)
    {
        hEdgeSortBufSize = numHEdges + 1;
        hEdgeSortBuf =
            M_Realloc(hEdgeSortBuf, hEdgeSortBufSize * sizeof(bsp_hedge_t*));
    }
}

static boolean C_DECL clockwiseLeaf(binarytree_t* tree, void* data)
{
    if(BinaryTree_IsLeaf(tree))
    {
        bspleafdata_t* leaf = (bspleafdata_t*) BinaryTree_GetData(tree);
        double midPoint[2] = { 0, 0 };
        bsp_hedge_t* hEdge;
        size_t total;

        getAveragedCoords(leaf->hEdges, &midPoint[VX], &midPoint[VY]);

        // Count half-edges.
        total = 0;
        for(hEdge = leaf->hEdges; hEdge; hEdge = hEdge->next)
            total++;

        // Ensure the sort buffer is large enough.
        prepareHEdgeSortBuffer(total);

        clockwiseOrder(&leaf->hEdges, total, midPoint[VX], midPoint[VY]);
        renumberLeafHEdges(leaf, data);

        // Do some sanity checks.
        sanityCheckClosed(leaf);
        sanityCheckSameSector(leaf);
        if(!sanityCheckHasRealHEdge(leaf))
        {
            Con_Error("BSP Leaf #%p has no linedef-linked half-edge!", leaf);
        }
    }

    return true; // Continue traversal.
}

void ClockwiseBspTree(binarytree_t* rootNode)
{
    uint curIndex;

    hEdgeSortBufSize = 0;
    hEdgeSortBuf = NULL;

    curIndex = 0;
    BinaryTree_PostOrder(rootNode, clockwiseLeaf, &curIndex);

    // Free temporary storage.
    if(hEdgeSortBuf)
    {
        M_Free(hEdgeSortBuf);
        hEdgeSortBuf = NULL;
    }
}

static void createBSPLeafWorker(bspleafdata_t* leaf, superblock_t* block)
{
    uint num;

    while(block->hEdges)
    {
        bsp_hedge_t* cur = block->hEdges;

        // Un-link first half-edge from the block.
        block->hEdges = cur->next;

        // Link it into head of the leaf's list.
        cur->next = leaf->hEdges;
        cur->block = NULL;

        leaf->hEdges = cur;
    }

    // Recursively handle sub-blocks.
    for(num = 0; num < 2; ++num)
    {
        superblock_t* a = block->subs[num];

        if(a)
        {
            createBSPLeafWorker(leaf, a);

            if(a->realNum + a->miniNum > 0)
                Con_Error("createBspLeafWorker: child %d not empty!", num);

            BSP_SuperBlockDestroy(a);
            block->subs[num] = NULL;
        }
    }

    block->realNum = block->miniNum = 0;
}

static __inline bspleafdata_t* allocBSPLeaf(void)
{
    return M_Malloc(sizeof(bspleafdata_t));
}

static __inline void freeBSPLeaf(bspleafdata_t* leaf)
{
    M_Free(leaf);
}

bspleafdata_t* BSPLeaf_Create(void)
{
    bspleafdata_t* leaf = allocBSPLeaf();

    leaf->hEdges = NULL;

    return leaf;
}

void BSPLeaf_Destroy(bspleafdata_t* leaf)
{
    bsp_hedge_t* cur, *np;

    if(!leaf) return;

    cur = leaf->hEdges;
    while(cur)
    {
        np = cur->next;
        BSP_HEdge_Destroy(cur);
        cur = np;
    }

    freeBSPLeaf(leaf);
}

/**
 * Create a new leaf from a list of half-edges.
 */
static bspleafdata_t* createBSPLeaf(superblock_t* hEdgeList)
{
    bspleafdata_t* leaf = BSPLeaf_Create();

    // Link the half-edges into the new leaf.
    createBSPLeafWorker(leaf, hEdgeList);

    return leaf;
}

boolean BuildNodes(superblock_t* hEdgeList, binarytree_t** parent, size_t depth,
    cutlist_t* cutList)
{
    binarytree_t* subTree;
    bspnodedata_t* node;
    superblock_t* hEdgeSet[2];
    bspleafdata_t* leaf;
    boolean builtOK = false;
    bspartition_t partition;

    *parent = NULL;

    /*DEBUG_Message(("Build: Begun @ %lu\n", (unsigned long) depth));
#if _DEBUG
    BSP_PrintSuperblockHEdges(hEdgeList);
#endif*/

    // Pick the next partition to use.
    if(!BSP_PickPartition(hEdgeList, depth, &partition))
    {
        // No partition required, already convex.
        //DEBUG_Message(("BuildNodes: Convex.\n"));

        leaf = createBSPLeaf(hEdgeList);
        *parent = BinaryTree_Create(leaf);
        return true;
    }

    //DEBUG_Message(("BuildNodes: Partition %p (%1.0f,%1.0f) -> (%1.0f,%1.0f).\n",
    //               best, best->v[0]->V_pos[VX], best->v[0]->V_pos[VY],
    //               best->v[1]->V_pos[VX], best->v[1]->V_pos[VY]));

    // Create left and right super blocks.
    hEdgeSet[RIGHT] = (superblock_t *) BSP_SuperBlockCreate();
    hEdgeSet[LEFT]  = (superblock_t *) BSP_SuperBlockCreate();

    // Copy the bounding box of the edge list to the superblocks.
    M_CopyBox(hEdgeSet[LEFT]->bbox, hEdgeList->bbox);
    M_CopyBox(hEdgeSet[RIGHT]->bbox, hEdgeList->bbox);

    // Divide the half-edges into two lists: left & right.
    BSP_PartitionHEdges(hEdgeList, &partition, hEdgeSet[RIGHT], hEdgeSet[LEFT], cutList);
    BSP_CutListEmpty(cutList);

    node = M_Calloc(sizeof(bspnodedata_t));
    *parent = BinaryTree_Create(node);

    BSP_FindNodeBounds(node, hEdgeSet[RIGHT], hEdgeSet[LEFT]);

    node->partition.x = partition.x;
    node->partition.y = partition.y;
    node->partition.dX = partition.dX;
    node->partition.dY = partition.dY;

    builtOK = BuildNodes(hEdgeSet[RIGHT], &subTree, depth + 1, cutList);
    BinaryTree_SetChild(*parent, RIGHT, subTree);
    BSP_SuperBlockDestroy(hEdgeSet[RIGHT]);

    if(builtOK)
    {
        builtOK = BuildNodes(hEdgeSet[LEFT], &subTree, depth + 1, cutList);
        BinaryTree_SetChild(*parent, LEFT, subTree);
    }

    BSP_SuperBlockDestroy(hEdgeSet[LEFT]);

    return builtOK;
}