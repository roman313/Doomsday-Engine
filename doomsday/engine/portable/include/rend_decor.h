/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2010 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2010 Daniel Swanson <danij@dengine.net>
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
 * Surface Decorations.
 */

#ifndef LIBDENG_RENDER_DECOR_H
#define LIBDENG_RENDER_DECOR_H

extern byte     useDecorations[NUM_DECORTYPES];
extern float    decorMaxDist;  // No decorations are visible beyond this.
extern float    decorLightBrightFactor;
extern float    decorLightFadeAngle;

void            Rend_DecorRegister(void);

void            Rend_DecorInit(void);

void            Rend_InitDecorationsForFrame(void);
void            Rend_AddLuminousDecorations(void);
void            Rend_ProjectDecorations(void);

#endif /* LIBDENG_RENDER_DECOR_H */
