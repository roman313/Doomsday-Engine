/**\file
 *\section Copyright and License Summary
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2006 Jaakko Keränen <skyjake@dengine.net>
 *\author Copyright © 2006 Jamie Jones <yagisan@dengine.net>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

/*
 * dd_types.h: Type Definitions
 */

#ifndef __DOOMSDAY_TYPES_H__
#define __DOOMSDAY_TYPES_H__

#ifdef UNIX
#  include <sys/types.h>
#endif

#ifndef _MSC_VER
#include <stdint.h>	// Not MSVC so use C99 standard header
#else
/* MSVC must define them ouselves.
ISO C9x Integer types - not all of them though, just what we need
If we need more the this, best of taking the header from MinGW for MSVC users */

/* 7.18.1.1  Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
typedef long long  int64_t;
typedef unsigned long long   uint64_t;

/* 7.18.1.2  Minimum-width integer types */
typedef signed char int_least8_t;
typedef unsigned char   uint_least8_t;
typedef short  int_least16_t;
typedef unsigned short  uint_least16_t;
typedef int  int_least32_t;
typedef unsigned   uint_least32_t;
typedef long long  int_least64_t;
typedef unsigned long long   uint_least64_t;

/*  7.18.1.3  Fastest minimum-width integer types 
 *  Not actually guaranteed to be fastest for all purposes
 *  Here we use the exact-width types for 8 and 16-bit ints. 
 */
typedef char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef short  int_fast16_t;
typedef unsigned short  uint_fast16_t;
typedef int  int_fast32_t;
typedef unsigned  int  uint_fast32_t;
typedef long long  int_fast64_t;
typedef unsigned long long   uint_fast64_t;

/* 7.18.1.4  Integer types capable of holding object pointers */
typedef int intptr_t;
typedef unsigned uintptr_t;

/* 7.18.1.5  Greatest-width integer types */
typedef long long  intmax_t;
typedef unsigned long long   uintmax_t;

#endif

// The C_DECL macro, used with functions.
#ifndef C_DECL
#  if defined(WIN32)
#    define C_DECL __cdecl
#  elif defined(UNIX)
#    define C_DECL
#  endif
#endif

#ifndef UNIX
typedef unsigned int	uint;
typedef unsigned short	ushort;
typedef unsigned int	size_t;
#endif

typedef int				fixed_t;
typedef unsigned int	angle_t;
typedef int				spritenum_t;
typedef unsigned int	ident_t;
typedef unsigned short	nodeindex_t;
typedef unsigned short	thid_t;
typedef unsigned char	byte;
typedef double			timespan_t;
typedef char			filename_t[256];

typedef struct directory_s {
	int             drive;
	filename_t      path;
} directory_t;

#ifdef __cplusplus
#  define boolean			int
#else							// Plain C.
#  ifndef __BYTEBOOL__
#    define __BYTEBOOL__
#  endif
typedef enum ddboolean_e { false, true } ddboolean_t;
#  define boolean			ddboolean_t
#endif

#define BAMS_BITS	16

#if BAMS_BITS == 32
typedef unsigned long binangle_t;
#elif BAMS_BITS == 16
typedef unsigned short binangle_t;
#else
typedef unsigned char binangle_t;
#endif

#define DDMAXCHAR	((char)0x7f)
#define DDMAXSHORT	((short)0x7fff)
#define DDMAXINT	((int)0x7fffffff)	// max pos 32-bit int
#define DDMAXLONG	((long)0x7fffffff)

#define DDMINCHAR	((char)0x80)
#define DDMINSHORT	((short)0x8000)
#define DDMININT 	((int)0x80000000)	// max negative 32-bit integer
#define DDMINLONG	((long)0x80000000)

#endif
