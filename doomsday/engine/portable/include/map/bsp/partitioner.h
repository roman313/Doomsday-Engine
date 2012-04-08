/**
 * @file partitioner.h
 * BSP space partitioner. @ingroup bsp
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

#ifndef LIBDENG_BSP_PARTITIONER
#define LIBDENG_BSP_PARTITIONER

#include "p_mapdata.h"

#include "map/bsp/bsptreenode.h"
#include "map/bsp/bsphedgeinfo.h"
#include "map/bsp/linedefinfo.h"
#include "map/bsp/vertexinfo.h"

#include <vector>
#include <list>

#define ET_prev             link[0]
#define ET_next             link[1]
#define ET_edge             hedges

// An edge tip is where an edge meets a vertex.
struct HEdgeTip
{
    // Link in list. List is kept in ANTI-clockwise order.
    HEdgeTip* link[2]; // {prev, next};

    /// Angle that line makes at vertex (degrees; 0 is E, 90 is N).
    double angle;

    // Half-edge on each side of the edge. Left is the side of increasing
    // angles, right is the side of decreasing angles. Either can be NULL
    // for one sided edges.
    struct hedge_s* hedges[2];

    HEdgeTip() : angle()
    {
        link[0] = 0;
        link[1] = 0;
        hedges[0] = 0;
        hedges[1] = 0;
    }
};

namespace de {
namespace bsp {

const double IFFY_LEN = 4.0;

/// Smallest distance between two points before being considered equal.
const double DIST_EPSILON = (1.0 / 128.0);

/// Smallest difference between two angles before being considered equal (in degrees).
const double ANG_EPSILON = (1.0 / 1024.0);

struct HEdgeIntercept;
class HPlane;
class HPlaneIntercept;
class SuperBlock;
class SuperBlockmap;

/**
 * @algorithm High-level description (courtesy of Raphael Quinet)
 *   1 - Create one Seg for each SideDef: pick each LineDef in turn.  If it
 *       has a "first" SideDef, then create a normal Seg.  If it has a
 *       "second" SideDef, then create a flipped Seg.
 *   2 - Call CreateNodes with the current list of Segs.  The list of Segs is
 *       the only argument to CreateNodes.
 *   3 - Save the Nodes, Segs and BspLeafs to disk.  Start with the leaves of
 *       the Nodes tree and continue up to the root (last Node).
 *
 * CreateNodes does the following:
 *   1 - Pick a nodeline amongst the Segs (minimize the number of splits and
 *       keep the tree as balanced as possible).
 *   2 - Move all Segs on the right of the nodeline in a list (segs1) and do
 *       the same for all Segs on the left of the nodeline (in segs2).
 *   3 - If the first list (segs1) contains references to more than one
 *       Sector or if the angle between two adjacent Segs is greater than
 *       180 degrees, then call CreateNodes with this (smaller) list.
 *       Else, create a BspLeaf with all these Segs.
 *   4 - Do the same for the second list (segs2).
 *   5 - Return the new node (its two children are already OK).
 *
 * Each time CreateBspLeaf is called, the Segs are put in a global list.
 * When there is no more Seg in CreateNodes' list, then they are all in the
 * global list and ready to be saved to disk.
 */
class Partitioner
{
public:
    Partitioner(GameMap* _map, uint* numEditableVertexes,
                Vertex*** editableVertexes, int _splitCostFactor=7);
    ~Partitioner();

    Partitioner& setSplitCostFactor(int factor);

    bool build();

    BspTreeNode* root() const;

    /**
     * Retrieve the number of BspNodes owned by this Partitioner. When the
     * build completes this number will be the total number of BspNodes that
     * were produced during that process. Note that as BspNode ownership is
     * claimed this number will decrease respectively.
     *
     * @return  Current number of BspNodes owned by this Partitioner.
     */
    uint numNodes();

    /**
     * Retrieve the number of BspLeafs owned by this Partitioner. When the
     * build completes this number will be the total number of BspLeafs that
     * were produced during that process. Note that as BspLeaf ownership is
     * claimed this number will decrease respectively.
     *
     * @return  Current number of BspLeafs owned by this Partitioner.
     */
    uint numLeafs();

    /**
     * Retrieve the number of HEdges owned by this Partitioner. When the build
     * completes this number will be the total number of half-edges that were
     * produced during that process. Note that as BspLeaf ownership is claimed
     * this number will decrease respectively.
     *
     * @return  Current number of HEdges owned by this Partitioner.
     */
    uint numHEdges();

    /**
     * Retrieve the total number of Vertexes produced during the build process.
     */
    uint numVertexes();

    /**
     * Retrieve the vertex with specified @a index. If the index is not valid
     * this will result in fatal error. The caller should ensure the index is
     * within valid range using Partitioner::numVertexes()
     */
    Vertex const& vertex(uint index);

private:
    struct Instance;
    Instance* d;
};

} // namespace bsp
} // namespace de

#endif /// LIBDENG_BSP_PARTITIONER
