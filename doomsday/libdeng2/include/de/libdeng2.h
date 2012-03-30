/*
 * The Doomsday Engine Project -- libdeng2
 *
 * Copyright (c) 2011 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#ifndef LIBDENG2_H
#define LIBDENG2_H

/**
 * @file Common definitions for libdeng2.
 */

#ifdef __cplusplus
#  define DENG2_USE_QT
#endif

#if defined(__x86_64__) || defined(__x86_64) || defined(_LP64)
#  define DENG2_64BIT
#endif

#ifdef DENG2_USE_QT
#include <QtCore/qglobal.h>
#endif

#ifndef _MSC_VER
#  include <stdint.h> // Not MSVC so use C99 standard header
#endif

#include <assert.h>

/*
 * The DENG2_PUBLIC macro is used to declare symbols for exporting. It must
 * be applied in all exported classes and functions. DEF files are not used for
 * exporting symbols out of libdeng2.
 */
#if defined(_WIN32) && defined(_MSC_VER)
#  ifdef __DENG2__
// This is defined when compiling the library.
#    define DENG2_PUBLIC __declspec(dllexport)
#  else
#    define DENG2_PUBLIC __declspec(dllimport)
#  endif
#else
// No need to use any special declarators.
#  define DENG2_PUBLIC
#endif

#ifndef NDEBUG
#  define DENG2_DEBUG
#  ifdef DENG2_USE_QT
#    define DENG2_ASSERT(x) Q_ASSERT(x)
#  else
#    define DENG2_ASSERT(x) assert(x)
#  endif
#else
#  define DENG2_NO_DEBUG
#  define DENG2_ASSERT(x)
#endif

/**
 * Macro for defining an opaque type in the C wrapper API.
 */
#define DENG2_OPAQUE(Name) \
    struct Name ## _s; \
    typedef struct Name ## _s Name;

/**
 * Macro for converting an opaque wrapper type to a de::type.
 * Asserts that the object really exists (not null).
 */
#define DENG2_SELF(Type, Var) \
    DENG2_ASSERT(Var != 0); \
    de::Type* self = reinterpret_cast<de::Type*>(Var);

/**
 * Macro for iterating through an STL container.
 *
 * @param Iter          Name of the iterator variable. Scope is limited to the for loop.
 * @param ContainerRef  Container.
 * @param IterClass     Iterator type.
 */
#define DENG2_FOR_EACH(Iter, ContainerRef, IterClass) \
    for(IterClass Iter = (ContainerRef).begin(); Iter != (ContainerRef).end(); ++Iter)

#ifdef __cplusplus
namespace de {

/**
 * @defgroup types Basic Types
 * Basic numeric data types.
 */

//@{
/// @ingroup types
typedef qint8   dchar;      ///< 8-bit signed integer.
typedef quint8  dbyte;      ///< 8-bit unsigned integer.
typedef quint8  duchar;     ///< 8-bit unsigned integer.
typedef dchar   dint8;      ///< 8-bit signed integer.
typedef dbyte   duint8;     ///< 8-bit unsigned integer.
typedef qint16  dint16;     ///< 16-bit signed integer.
typedef quint16 duint16;    ///< 16-bit unsigned integer.
typedef dint16  dshort;     ///< 16-bit signed integer.
typedef duint16 dushort;    ///< 16-bit unsigned integer.
typedef qint32  dint32;     ///< 32-bit signed integer.
typedef quint32 duint32;    ///< 32-bit unsigned integer.
typedef dint32  dint;       ///< 32-bit signed integer.
typedef duint32 duint;      ///< 32-bit unsigned integer.
typedef qint64  dint64;     ///< 64-bit signed integer.
typedef quint64 duint64;    ///< 64-bit unsigned integer.
typedef float   dfloat;     ///< 32-bit floating point number.
typedef qreal   ddouble;    ///< 64-bit floating point number.
typedef quint64 dsize;

// Pointer-integer conversion (used for legacy code).
#ifdef DENG2_64BIT
typedef duint64 dintptr;
#else
typedef duint32 dintptr;
#endif
//@}

} // namespace de

#include "Error"

#else // !__cplusplus

/*
 * Data types for C APIs.
 */
typedef unsigned char   dbyte;
typedef unsigned int    duint;  // 32-bit

#ifdef DENG2_64BIT
typedef uint64_t        dsize;  // 64-bit size
#else
typedef unsigned int    dsize;  // 32-bit size
#endif

#endif

#endif // LIBDENG2_H