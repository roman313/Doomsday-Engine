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

#ifndef LIBDENG2_MATH_H
#define LIBDENG2_MATH_H

#include <de/libdeng2.h>
#include <cmath>

#ifdef min
#   undef min
#endif

#ifdef max
#   undef max
#endif

namespace de {

#undef PI
ddouble const PI = 3.1415926535897932384626433832795028841971693993751;
ddouble const EPSILON = 1.0e-7;
dfloat const FLOAT_EPSILON = 1.0e-5f;

/// Absolute value.
template <typename Type>
inline Type abs(Type const &a) {
    if(a < 0.0) {
        return -a;
    }
    return a;
}

// Special case, this is never negative.
inline duint abs(duint const &a) {
    return a;
}

/// Minimum of two values.
template <typename Type>
inline Type const &min(Type const &a, Type const &b) {
    return (a < b? a : b);
}

/// Maximum of two values.
template <typename Type>
inline Type const &max(Type const &a, Type const &b) {
    return (a > b? a : b);
}

/// Clamp value within range.
template <typename Type>
inline Type clamp(Type const &low, Type const &value, Type const &high) {
    return min(max(value, low), high);
}

/// Wrap value within range [low, high).
template <typename Type>
Type wrap(Type value, Type const &low, Type const &high) {
    Type const range = high - low;
    while(value < low) value += range;
    while(value >= high) value -= range;
    return value;
}

template <typename Type>
inline Type squared(Type const &value) { return value * value; }

template <typename Type>
inline Type round(dfloat const &value) {
    return Type(std::floor(value + 0.5f));
}

template <typename Type>
inline Type round(ddouble const &value) {
    return Type(std::floor(value + 0.5));
}

inline dint32 floor(dfloat const &value) {
    return dint32(std::floor(value));
}

inline dint64 floor(ddouble const &value) {
    return dint64(std::floor(value));
}

inline dint32 ceil(dfloat const &value) {
    return dint32(std::ceil(value));
}

inline dint64 ceil(ddouble const &value) {
    return dint64(std::ceil(value));
}

/// Compare two single-precision floating-point values for equality,
/// with the precision of FLOAT_EPSILON.
inline bool fequal(dfloat a, dfloat b) {
    return abs(a - b) < FLOAT_EPSILON;
}

/// Compare two double-precision floating-point values for equality,
/// with the precision of EPSILON.
inline bool fequal(ddouble a, ddouble b) {
    return abs(a - b) < EPSILON;
}

template <typename Type>
inline Type degreeToRadian(Type const &degree) {
    return degree / Type(180) * PI;
}

template <typename Type>
inline Type radianToDegree(Type const &radian) {
    return radian / PI * Type(180);
}

/// General comparison function.
template <typename Type>
inline dint cmp(Type const &a, Type const &b) {
    if(a < b) return -1;
    if(a > b) return 1;
    return 0;
}

} // namespace de

#endif /* LIBDENG2_MATH_H */
