/*
 * The Doomsday Engine Project -- libdeng2
 *
 * Copyright (c) 2004-2009 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#ifndef LIBDENG2_READER_HH
#define LIBDENG2_READER_HH

#include "../libdeng2.h"
#include "../IByteArray"
#include "../ByteOrder"

namespace de {

class Block;
class String;
class IReadable;
class FixedByteArray;

/**
 * Provides a protocol for reading data from a byte array object (anything with
 * a IByteArray interface). Byte order defaults to little-endian but can be
 * changed to big-endian.
 *
 * @ingroup data
 */
class DENG2_PUBLIC Reader
{
public:
    /**
     * Constructs a new reader.
     *
     * @param source     Byte array containing the data to be read.
     * @param byteOrder  Byte order to use. The byte order defaults to network
     *                   (big-endian) byte order.
     * @param offset     Offset in @a source where to start reading.
     */
    Reader(const IByteArray& source, const ByteOrder& byteOrder = littleEndianByteOrder,
        IByteArray::Offset offset = 0);

    //@{ Read a number from the source buffer, in network byte order.
    Reader& operator >> (char& byte);
    Reader& operator >> (dchar& byte);
    Reader& operator >> (duchar& byte);
    Reader& operator >> (dint16& word);
    Reader& operator >> (duint16& word);
    Reader& operator >> (dint32& dword);
    Reader& operator >> (duint32& dword);
    Reader& operator >> (dint64& qword);
    Reader& operator >> (duint64& qword);
    Reader& operator >> (dfloat& value);
    Reader& operator >> (ddouble& value);
    //@}

    /// Reads a string from the source buffer.
    Reader& operator >> (String& text);

    /// Reads a sequence bytes from the source buffer.
    Reader& operator >> (IByteArray& byteArray);

    /**
     * Reads a fixed-size sequence of bytes from the source buffer.
     * The size of the sequence is determined by the size of
     * @a fixedByteArray.
     *
     * @param fixedByteArray  Destination buffer.
     *
     * @return  Reference to the Reader.
     */
    Reader& operator >> (FixedByteArray& fixedByteArray);

    /// Reads a Block from the source buffer.
    Reader& operator >> (Block& block);

    /// Reads a serializable object from the source buffer.
    Reader& operator >> (IReadable& readable);

    /**
     * Reads bytes from the source buffer until a specified delimiter
     * value is encountered. The delimiter is included as part of
     * the read data.
     *
     * @param byteArray  Destination buffer.
     * @param delimiter  Delimiter value.
     */
    Reader& readUntil(IByteArray& byteArray, IByteArray::Byte delimiter = 0);

    /**
     * Returns the source byte array of the reader.
     */
    const IByteArray& source() const {
        return _source;
    }

    /**
     * Returns the offset used by the reader.
     */
    IByteArray::Offset offset() const {
        return _offset;
    }

    /**
     * Move to a specific position in the source data.
     *
     * @param offset  Offset to move to.
     */
    void setOffset(IByteArray::Offset offset) {
        _offset = offset;
    }

    /**
     * Moves the reader offset forward by a number of bytes.
     *
     * @param count  Number of bytes to move forward.
     */
    void seek(dint count);

    /**
     * Rewinds the read offset by a number of bytes.
     *
     * @param count  Number of bytes to move backward.
     */
    void rewind(dint count);

    /**
     * Returns the byte order of the writer.
     */
    const ByteOrder& byteOrder() const {
        return _convert;
    }

private:
    const IByteArray& _source;
    IByteArray::Offset _offset;
    const ByteOrder& _convert;
};

} // namespace de

#endif /* LIBDENG2_READER_HH */