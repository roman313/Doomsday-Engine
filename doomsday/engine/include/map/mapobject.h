/** @file mapobject.h Base class for all map data objects.
 * @ingroup map
 *
 * @authors Copyright © 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2013 Daniel Swanson <danij@dengine.net>
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

#ifndef LIBDENG_MAPOBJECT_H
#define LIBDENG_MAPOBJECT_H

#include "dd_share.h"

#ifndef __cplusplus
#  error "map/mapobject.h requires C++"
#endif

#include <QList>

namespace de {

/**
 * Base class for all map data objects.
 *
 * @todo This shouldn't be confused with mobjs. Figure out a better name. -jk
 */
class MapObject
{
public:
    MapObject(int t = DMU_NONE) : _type(t) {}

    virtual ~MapObject() {}

    int type() const
    {
        return _type;
    }

private:
    int _type;
};

/**
 * Collection of owned map objects with type @a Type.
 */
template <typename Type>
class MapObjectList : public QList<MapObject *>
{
    typedef QList<MapObject *> Super;

public:
    MapObjectList() {}

    /**
     * Deletes all the objects in the list.
     */
    virtual ~MapObjectList()
    {
        clear();
    }

    void clear()
    {
        // Delete all the objects.
        for(iterator i = begin(); i != end(); ++i)
        {
            delete *i;
        }
        Super::clear();
    }

    void clearAndResize(int count)
    {
        clear();
        while(count-- > 0)
        {
            append(new Type);
        }
    }

    int indexOf(Type const *t, int from = 0) const
    {
        return Super::indexOf(const_cast<Type *>(t), from);
    }

    Type &operator [] (int index)
    {
        DENG_ASSERT(dynamic_cast<Type *>(at(index)) != 0);
        return *static_cast<Type *>(Super::at(index));
    }

    Type const &operator [] (int index) const
    {
        DENG_ASSERT(dynamic_cast<Type const *>(at(index)) != 0);
        return *static_cast<Type const *>(Super::at(index));
    }
};

} // namespace de

#endif // LIBDENG_MAPOBJECT_H