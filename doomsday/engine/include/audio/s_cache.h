/**\file s_cache.h
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2012 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2012 Daniel Swanson <danij@dengine.net>
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
 * Sound Sample Cache.
 */

#ifndef LIBDENG_SOUND_CACHE_H
#define LIBDENG_SOUND_CACHE_H

void Sfx_InitCache(void);
void Sfx_ShutdownCache(void);

sfxsample_t* Sfx_Cache(int id);
void Sfx_CacheHit(int id);
uint Sfx_GetSoundLength(int id);
void Sfx_GetCacheInfo(uint* cacheBytes, uint* sampleCount);

#endif /* LIBDENG_SOUND_CACHE_H */