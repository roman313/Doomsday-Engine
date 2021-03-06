/*
 * The Doomsday Engine Project -- libdeng2
 *
 * Copyright (c) 2004-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBDENG2_WAITABLE_H
#define LIBDENG2_WAITABLE_H

#include "../libdeng2.h"
#include "../Time"

#include <QSemaphore>

namespace de {

/**
 * Semaphore that allows objects to be waited on.
 *
 * @ingroup data
 */
class DENG2_PUBLIC Waitable
{
public:
    /// wait() failed due to timing out before the resource is secured. @ingroup errors
    DENG2_ERROR(TimeOutError);

    /// wait() or waitTime() failed to secure the resource. @ingroup errors
    DENG2_ERROR(WaitError);

public:
    Waitable(duint initialValue = 0);
    virtual ~Waitable();

    /// Resets the semaphore to zero.
    void reset();

    /// Wait until the resource becomes available. Waits indefinitely.
    void wait() const;

    /// Wait for the specified period of time to secure the
    /// resource.  If timeout occurs, an exception is thrown.
    void wait(TimeDelta const &timeOut) const;

    /// Mark the resource as available by incrementing the
    /// semaphore value.
    void post() const;

private:
    /// Pointer to the internal semaphore data.
    mutable QSemaphore _semaphore;
};

}

#endif /* LIBDENG2_WAITABLE_H */
