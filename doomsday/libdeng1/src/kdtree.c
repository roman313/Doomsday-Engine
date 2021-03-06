/** @file kdtree.c Kd-Tree data structure implementation.
 * @ingroup data
 *
 * Based on glBSP 2.24 (in turn, based on BSP 2.3), which is hosted on
 * SourceForge: http://sourceforge.net/projects/glbsp/
 *
 * @authors Copyright © 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#include <assert.h>
#include <stdlib.h>
#include <de/memory.h>

#include "kdtree.h"

struct kdtreenode_s
{
    /// KdTree instance which owns this tree node.
    struct kdtree_s *kdTree;

    /// Parent of this (sub)tree else @c NULL.
    struct kdtreenode_s *parent;

    /// Subtree of this (sub)tree else @c NULL.
    struct kdtreenode_s *subs[2];

    /// Coordinates for this subtree, from lower-left to upper-right corner.
    /// Pseudo-inclusive, i.e (x,y) is inside block if and only if:
    ///     minX <= x < maxX and minY <= y < maxY.
    AABox aaBox;

    /// User data associated with this (sub)tree else @c NULL.
    void *userData;
};

static KdTreeNode *KdTreeNode_New(KdTree *kdTree, const AABox *bounds);
static KdTreeNode *KdTreeNode_NewWithUserData(KdTree *kdTree, const AABox *bounds, void *userData);

static int KdTreeNode_PostTraverse(KdTreeNode *kdn, int (*callback)(KdTreeNode *, void *), void *parameters);

struct kdtree_s {
    KdTreeNode *root;
};

KdTree *KdTree_New(const AABox *bounds)
{
    KdTree *kd = (KdTree *) M_Malloc(sizeof *kd);
    kd->root = KdTreeNode_New(kd, bounds);
    return kd;
}

static int deleteKdTreeNode(KdTreeNode *kdn, void *parameters)
{
    DENG_UNUSED(parameters);

    KdTreeNode_Delete(kdn);
    return false; // Continue iteration.
}

void KdTree_Delete(KdTree *kd)
{
    DENG_ASSERT(kd);
    KdTree_PostTraverse(kd, deleteKdTreeNode);
    M_Free(kd);
}

KdTreeNode *KdTree_Root(KdTree *kd)
{
    DENG_ASSERT(kd);
    return kd->root;
}

int KdTree_PostTraverse2(KdTree *kd, int(*callback)(KdTreeNode*, void*),
    void *parameters)
{
    return KdTreeNode_PostTraverse(kd->root, callback, parameters);
}

int KdTree_PostTraverse(KdTree *kd, int(*callback)(KdTreeNode*, void*))
{
    return KdTree_PostTraverse2(kd, callback, NULL/*no parameters*/);
}

static KdTreeNode *KdTreeNode_NewWithUserData(KdTree *kdTree, const AABox *bounds, void *userData)
{
    KdTreeNode *kdn = (KdTreeNode *) M_Calloc(sizeof *kdn);
    kdn->kdTree = kdTree;
    memcpy(&kdn->aaBox, bounds, sizeof(kdn->aaBox));
    kdn->userData = userData;
    return kdn;
}

static KdTreeNode *KdTreeNode_New(KdTree *kdTree, const AABox *bounds)
{
    return KdTreeNode_NewWithUserData(kdTree, bounds, NULL/*no user data*/);
}

void KdTreeNode_Delete(KdTreeNode *kdn)
{
    DENG_ASSERT(kdn);
    if(kdn->parent)
    {
        if(kdn->parent->subs[0] == kdn)
        {
            kdn->parent->subs[0] = 0;
        }
        else if(kdn->parent->subs[1] == kdn)
        {
            kdn->parent->subs[1] = 0;
        }
    }
    free(kdn);
}

KdTree *KdTreeNode_KdTree(KdTreeNode *kdn)
{
    DENG_ASSERT(kdn);
    return kdn->kdTree;
}

const AABox *KdTreeNode_Bounds(KdTreeNode *kdn)
{
    DENG_ASSERT(kdn);
    return &kdn->aaBox;
}

void *KdTreeNode_UserData(KdTreeNode *kdn)
{
    DENG_ASSERT(kdn);
    return kdn->userData;
}

KdTreeNode *KdTreeNode_SetUserData(KdTreeNode *kdn, void *userData)
{
    DENG_ASSERT(kdn);
    kdn->userData = userData;
    return kdn;
}

KdTreeNode *KdTreeNode_Parent(KdTreeNode *kdn)
{
    DENG_ASSERT(kdn);
    return kdn->parent;
}

KdTreeNode *KdTreeNode_Child(KdTreeNode *kdn, int left)
{
    DENG_ASSERT(kdn);
    return kdn->subs[left?1:0];
}

KdTreeNode *KdTreeNode_AddChild(KdTreeNode *kdn, double distance, int vertical, int left, void *userData)
{
    KdTreeNode *child;
    AABox sub;
    DENG_ASSERT(kdn);

    distance = MINMAX_OF(-1, distance, 1);
    if(distance < 0) distance = -distance;

    if(!vertical)
    {
        int division = (int) (kdn->aaBox.minX + 0.5 + distance * (kdn->aaBox.maxX - kdn->aaBox.minX));

        sub.minX = (left? division : kdn->aaBox.minX);
        sub.minY = kdn->aaBox.minY;

        sub.maxX = (left? kdn->aaBox.maxX : division);
        sub.maxY = kdn->aaBox.maxY;
    }
    else
    {
        int division = (int) (kdn->aaBox.minY + 0.5 + distance * (kdn->aaBox.maxY - kdn->aaBox.minY));

        sub.minX = kdn->aaBox.minX;
        sub.minY = (left? division : kdn->aaBox.minY);

        sub.maxX = kdn->aaBox.maxX;
        sub.maxY = (left? kdn->aaBox.maxY : division);
    }

    child = kdn->subs[left?1:0];
    if(!child)
    {
        child = kdn->subs[left?1:0] = KdTreeNode_New(kdn->kdTree, &sub);
        child->parent = kdn;
    }

    child->userData = userData;
    return child;
}

int KdTreeNode_Traverse2(KdTreeNode *kdn, int (*callback)(KdTreeNode *, void *), void *parameters)
{
    int num, result;
    DENG_ASSERT(kdn);

    if(!callback) return false; // Continue iteration.

    result = callback(kdn, parameters);
    if(result) return result;

    // Recursively handle subtrees.
    for(num = 0; num < 2; ++num)
    {
        KdTreeNode *child = kdn->subs[num];
        if(!child) continue;

        result = KdTreeNode_Traverse2(child, callback, parameters);
        if(result) return result;
    }

    return false; // Continue iteration.
}

int KdTreeNode_Traverse(KdTreeNode *kdn, int (*callback)(KdTreeNode *, void *))
{
    return KdTreeNode_Traverse2(kdn, callback, NULL/*no parameters*/);
}

static int KdTreeNode_PostTraverse(KdTreeNode *kdn, int (*callback)(KdTreeNode *, void *),
                                   void *parameters)
{
    int num, result;
    DENG_ASSERT(kdn);

    if(!callback) return false; // Continue iteration.

    // Recursively handle sub-blocks.
    for(num = 0; num < 2; ++num)
    {
        KdTreeNode *child = kdn->subs[num];
        if(!child) continue;

        result = KdTreeNode_PostTraverse(child, callback, parameters);
        if(result) return result;
    }

    result = callback(kdn, parameters);
    if(result) return result;

    return false; // Continue iteration.
}
