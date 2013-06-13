/** @file bspbuilder.h World Map BSP Builder.
 *
 * @authors Copyright © 2007-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef DENG_WORLD_MAP_BSPBUILDER
#define DENG_WORLD_MAP_BSPBUILDER

#include "world/map.h"
#include "world/bsp/bsptreenode.h"

namespace de {

/**
 * Map geometry partitioner and BSP tree builder.
 *
 * BspBuilder constructs a BSP object object tree for the specified map.
 *
 * @ingroup world
 */
class BspBuilder
{
public:
    /// Default cost factor attributed to splitting a half-edge.
    static const int DEFAULT_PARTITION_COST_HEDGESPLIT = 7;

public:
    /**
     * Create and configure a new BSP builder, initializing in preparation
     * for building a BSP for the specified @a map.
     *
     * @param map  Map to construct a BSP for. The caller must ensure that
     *             the map remains accessible until the build process has
     *             completed (ownership is unaffected).
     * @param splitCostFactor  Cost factor attributed to splitting a half-edge.
     */
    BspBuilder(Map const &map, int splitCostFactor = DEFAULT_PARTITION_COST_HEDGESPLIT);

    /**
     * Set the cost factor associated with splitting an existing half-edge.
     *
     * @param newFactor  New split cost factor.
     */
    void setSplitCostFactor(int newFactor);

    /**
     * Build the BSP for the given map.
     * @return  @c true= iff completed successfully.
     */
    bool buildBsp();

    /**
     * Retrieve a pointer to the root BinaryTree node for the constructed BSP.
     * Even if construction fails this will return a valid node.
     *
     * The only time upon which @c NULL is returned is if called prior to calling
     * BspBuilder::build()
     */
    BspTreeNode *root() const;

    /**
     * Retrieve the number of Segments owned by this Partitioner. When the build
     * completes this number will be the total number of line segments that were
     * produced during that process. Note that as BspLeaf ownership is claimed
     * this number will decrease respectively.
     *
     * @return  Current number of Segments owned by this Partitioner.
     */
    int numSegments();

    /**
     * Retrieve the number of BspLeafs owned by this Partitioner. When the
     * build completes this number will be the total number of BspLeafs that
     * were produced during that process. Note that as BspLeaf ownership is
     * claimed this number will decrease respectively.
     *
     * @return  Current number of BspLeafs owned by this Partitioner.
     */
    int numLeafs();

    /**
     * Retrieve the number of BspNodes owned by this Partitioner. When the
     * build completes this number will be the total number of BspNodes that
     * were produced during that process. Note that as BspNode ownership is
     * claimed this number will decrease respectively.
     *
     * @return  Current number of BspNodes owned by this Partitioner.
     */
    int numNodes();

    /**
     * Retrieve the total number of Vertexes produced during the build process.
     */
    int numVertexes();

    /**
     * Retrieve the vertex with specified @a index. If the index is not valid
     * this will result in fatal error. The caller should ensure the index is
     * within valid range using Partitioner::numVertexes()
     */
    Vertex &vertex(int idx);

    /**
     * Release ownership of the specified object.
     *
     * @param mapElement  Map data object to relinquish ownership of.
     */
    void take(MapElement *mapElement);

private:
    DENG2_PRIVATE(d)
};

} // namespace de

#endif // DENG_WORLD_MAP_BSPBUILDER