/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2009 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2009 Daniel Swanson <danij@dengine.net>
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

/**
 * version.h: Version numbering, naming etc.
 */

#ifndef __JDOOM64_VERSION_H__
#define __JDOOM64_VERSION_H__

#ifndef __JDOOM64__
#  error "Using jDoom64 headers without __JDOOM64__"
#endif

#ifndef JDOOM64_VER_ID
#  ifdef _DEBUG
#    define JDOOM64_VER_ID "+D Doomsday"
#  else
#    define JDOOM64_VER_ID "Doomsday"
#  endif
#endif

// Used to derive filepaths.
#define GAMENAMETEXT        "jdoom64"

// Presented to the user in dialogs, messages etc.
#define GAME_NICENAME       "jDoom64"
#define GAME_DETAILS        "jDoom64 is based on jDoom-1.15."

#define GAME_VERSION_TEXT   "0.8.1"
#define GAME_VERSION_TEXTLONG GAME_VERSION_TEXT " " __DATE__ " (" JDOOM64_VER_ID ")"
#define GAME_VERSION_NUMBER 0,8,1,0 // For WIN32 version info.

#endif
