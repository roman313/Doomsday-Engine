/*
 * The Doomsday Engine Project -- libdeng2
 *
 * Copyright (c) 2012-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#ifndef LIBDENG2_BYTEARRAYFILE_H
#define LIBDENG2_BYTEARRAYFILE_H

#include "../libdeng2.h"
#include "../File"
#include "../IByteArray"

namespace de {

/**
 * Reads from and writes to files that contain a random-access byte array
 * of data. This is an abstract base class for files that have this
 * property.
 *
 * When used as an I/O stream: reading from the stream outputs the entire
 * contents of the file, and writing to the stream appends new content to
 * the end of the file. Byte array files must be used is immutable mode;
 * the bytes are not removed from the stream by readers (i.e., when, say, a
 * native file is read, the bytes aren't removed from the file).
 *
 * @ingroup fs
 */
class DENG2_PUBLIC ByteArrayFile : public File, public IByteArray
{
protected:
    ByteArrayFile(String const &name = "") : File(name) {}

public:
    // Implements IIOStream.
    IOStream &operator << (IByteArray const &bytes);
    IIStream &operator >> (IByteArray &bytes);
    IIStream const &operator >> (IByteArray &bytes) const;
};

} // namespace de

#endif // LIBDENG2_BYTEARRAYFILE_H
