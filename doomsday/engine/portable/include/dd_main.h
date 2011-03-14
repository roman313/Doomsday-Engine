/**\file dd_main.h
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2011 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2011 Daniel Swanson <danij@dengine.net>
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
 * Engine Core
 */

#ifndef LIBDENG_MAIN_H
#define LIBDENG_MAIN_H

#include "dd_types.h"
#include "gameinfo.h"

// Verbose messages.
#define VERBOSE(code)   { if(verbose >= 1) { code; } }
#define VERBOSE2(code)  { if(verbose >= 2) { code; } }

extern int verbose;
extern FILE* outFile; // Output file for console messages.

extern filename_t ddBasePath;
extern directory_t ddRuntimeDir, ddBinDir;

extern char* gameStartupFiles; // A list of names of files to be autoloaded during startup, whitespace in between (in .cfg).

extern int isDedicated;

extern finaleid_t titleFinale;

#ifndef WIN32
extern GETGAMEAPI GetGameAPI;
#endif

int DD_EarlyInit(void);
int DD_Main(void);
void DD_CheckTimeDemo(void);
void DD_UpdateEngineState(void);

/**
 * Executes all the hooks of the given type. Bit zero of the return value
 * is set if a hook was executed successfully (returned true). Bit one is
 * set if all the hooks that were executed returned true.
 */
int DD_CallHooks(int hook_type, int parm, void* data);

/// @return  Unique identified of the plugin responding to active hook callback.
pluginid_t DD_PluginIdForActiveHook(void);

/**
 * Locate the address of the named, exported procedure in the plugin.
 */
void* DD_FindEntryPoint(pluginid_t pluginId, const char* fn);

int DD_GetInteger(int ddvalue);
void DD_SetInteger(int ddvalue, int parm);
void DD_SetVariable(int ddvalue, void* ptr);
void* DD_GetVariable(int ddvalue);

ddplayer_t* DD_GetPlayer(int number);

materialnum_t DD_MaterialForTextureIndex(uint index, texturenamespaceid_t texNamespace);

const char* value_Str(int val);

/**
 * @return  Ptr to the currently active GameInfo structure (always succeeds).
 */
gameinfo_t* DD_GameInfo(void);

/**
 * @return  Current number of GameInfo structures.
 */
int DD_GameInfoCount(void);

/**
 * @return  Ptr the GameInfo with the specified index else @c NULL if invalid.
 */
gameinfo_t* DD_GameInfoByIndex(int idx);

/**
 * Is this the special "null-game" object (not a real playable game).
 * \todo Implement a proper null-gameinfo object for this.
 */
boolean DD_IsNullGameInfo(gameinfo_t* info);

/**
 * Frees the info structures for all registered games.
 */
void DD_DestroyGameInfo(void);

D_CMD(Load);
D_CMD(Unload);
D_CMD(Reset);
D_CMD(PrintInfo);
D_CMD(ListGames);

#endif /* LIBDENG_MAIN_H */
