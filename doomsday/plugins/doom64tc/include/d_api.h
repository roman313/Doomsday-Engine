/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2008 Daniel Swanson <danij@dengine.net>
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
 * d_api.c: Doomsday API setup and interaction - Doom64TC specific
 */

#ifndef __D_API_H__
#define __D_API_H__

#ifndef __DOOM64TC__
#  error "Using Doom64TC headers without __DOOM64TC__"
#endif

#include "dd_api.h"

extern game_import_t gi;
extern game_export_t gx;

int             G_GetInteger(int id);
void           *G_GetVariable(int id);
game_export_t  *GetGameAPI(game_import_t *imports);

#endif
