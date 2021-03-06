/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2008 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2008 Daniel Swanson <danij@dengine.net>
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

#ifndef __S_SOUND__
#define __S_SOUND__

#ifndef __JDOOM__
#  error "Using jDoom headers without __JDOOM__"
#endif

#include "r_defs.h"

#include "sndidx.h"                // Sfx and music indices.

typedef enum {
    SORG_CENTER,
    SORG_FLOOR,
    SORG_CEILING
} sectorsoundorigin_t;

void            S_MapMusic(void);
void            S_SectorSound(sector_t *sector, int origin, int sound_id);

#endif
