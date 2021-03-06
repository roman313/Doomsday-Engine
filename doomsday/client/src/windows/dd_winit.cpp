/** @file dd_winit.cpp  Engine Initialization (Windows).
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2005-2013 Daniel Swanson <danij@dengine.net>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, see:
 * http://www.gnu.org/licenses</small>
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_DCOM
#define STRICT

#ifdef __CLIENT__
#  include <de/DisplayMode>
#endif

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "resource.h"

#include "de_base.h"
#include "de_graphics.h"
#include "de_console.h"
#include "de_system.h"
#include "de_play.h"
#include "de_network.h"
#include "de_misc.h"
#include "de_ui.h"

#include "filesys/fs_util.h"
#include "dd_winit.h"

application_t app;

#ifdef UNICODE
static LPWSTR convBuf;
static LPSTR utf8ConvBuf;
#endif

#ifdef UNICODE
LPCWSTR ToWideString(const char* str)
{
    // Determine the length of the output string.
    int wideChars = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);

    // Allocate the right amount of memory.
    int bufSize = wideChars * sizeof(wchar_t) + 1;
    convBuf = (LPWSTR) M_Realloc(convBuf, bufSize);
    memset(convBuf, 0, bufSize);

    MultiByteToWideChar(CP_ACP, 0, str, -1, convBuf, wideChars);

    return convBuf;
}

LPCSTR ToAnsiString(const wchar_t* wstr)
{
    // Determine how much memory is needed for the output string.
    int utfBytes = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, 0, 0, 0, 0);

    // Allocate the right amount of memory.
    utf8ConvBuf = (LPSTR) M_Realloc(utf8ConvBuf, utfBytes);

    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8ConvBuf, utfBytes, 0, 0);

    return utf8ConvBuf;
}
#endif

/**
 * \note GetLastError() should only be called when we *know* an error was thrown.
 * The result of calling this any other time is undefined.
 *
 * @return Ptr to a string containing a textual representation of the last
 * error thrown in the current thread else @c NULL.
 */
const char* DD_Win32_GetLastErrorMessage(void)
{
    static char* buffer = 0; /// @todo Never free'd!
    static size_t currentBufferSize = 0;

    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(), lpMsgBufLen;
    lpMsgBufLen = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 0, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, 0);
    if(!lpMsgBuf || lpMsgBufLen == 0)
        return "";

    if(!buffer || (size_t)(lpMsgBufLen+1+8) > currentBufferSize)
    {
        currentBufferSize = (size_t)(lpMsgBufLen+1+8);
        buffer = (char *) M_Realloc(buffer, currentBufferSize);
    }

    dd_snprintf(buffer, currentBufferSize, "#%-5d: ", (int)dw);

    // Continue splitting as long as there are parts.
    { char* part, *cursor = (char*)lpMsgBuf;
    while(*(part = M_StrTok(&cursor, "\n")))
        strcat(buffer, part);
    }

    // We're done with the system-allocated message.
    LocalFree(lpMsgBuf);
    return buffer;
}

static BOOL initDGL(void)
{
#ifdef __CLIENT__
    return (BOOL) Sys_GLPreInit();
#else
    return TRUE;
#endif
}

static void determineGlobalPaths(application_t* app)
{
    assert(app);

    // Where are we?
#if defined(DENG_LIBRARY_DIR)
#  if !defined(_DEBUG)
#pragma message("!!!WARNING: DENG_LIBRARY_DIR defined in non-debug build!!!")
#  endif
    {
    filename_t path;
    directory_t* temp;

    dd_snprintf(path, FILENAME_T_MAXLEN, "%s", DENG_LIBRARY_DIR);
    // Ensure it ends with a directory separator.
    F_AppendMissingSlashCString(path, FILENAME_T_MAXLEN);
    Dir_MakeAbsolutePath(path);
    temp = Dir_FromText(path);
    strncpy(ddBinPath, Str_Text(temp), FILENAME_T_MAXLEN);
    Dir_Delete(temp);
    }
#else
    {
        directory_t* temp;
#ifdef UNICODE
        wchar_t path[FILENAME_T_MAXLEN];
        GetModuleFileName(app->hInstance, path, FILENAME_T_MAXLEN);
        temp = Dir_FromText(ToAnsiString(path));
#else
        filename_t path;
        GetModuleFileName(app->hInstance, path, FILENAME_T_MAXLEN);
        temp = Dir_FromText(path);
#endif
        strncpy(ddBinPath, Dir_Path(temp), FILENAME_T_MAXLEN);
        Dir_Delete(temp);
    }
#endif

    // The -userdir option sets the working directory.
    if(CommandLine_CheckWith("-userdir", 1))
    {
        filename_t runtimePath;
        directory_t* temp;

        strncpy(runtimePath, CommandLine_NextAsPath(), FILENAME_T_MAXLEN);
        Dir_CleanPath(runtimePath, FILENAME_T_MAXLEN);
        // Ensure the path is closed with a directory separator.
        F_AppendMissingSlashCString(runtimePath, FILENAME_T_MAXLEN);

        temp = Dir_New(runtimePath);
        app->usingUserDir = Dir_SetCurrent(Dir_Path(temp));
        if(app->usingUserDir)
        {
            strncpy(ddRuntimePath, Dir_Path(temp), FILENAME_T_MAXLEN);
        }
        Dir_Delete(temp);
    }

    if(!app->usingUserDir)
    {
        // The current working directory is the runtime dir.
        directory_t* temp = Dir_NewFromCWD();
        Dir_SetCurrent(Dir_Path(temp));
        strncpy(ddRuntimePath, Dir_Path(temp), FILENAME_T_MAXLEN);
        Dir_Delete(temp);
    }

    if(CommandLine_CheckWith("-basedir", 1))
    {
        strncpy(ddBasePath, CommandLine_Next(), FILENAME_T_MAXLEN);
    }
    else
    {
        // The standard base directory is one level up from the bin dir.
        dd_snprintf(ddBasePath, FILENAME_T_MAXLEN, "%s../", ddBinPath);
    }
    Dir_CleanPath(ddBasePath, FILENAME_T_MAXLEN);
    Dir_MakeAbsolutePath(ddBasePath, FILENAME_T_MAXLEN);
    // Ensure it ends with a directory separator.
    F_AppendMissingSlashCString(ddBasePath, FILENAME_T_MAXLEN);
}

boolean DD_Win32_Init(void)
{
    BOOL failed = TRUE;

    memset(&app, 0, sizeof(app));
    app.hInstance = GetModuleHandle(NULL);

    // Initialize COM.
    CoInitialize(NULL);

    // Prepare the command line arguments.
    DD_InitCommandLine();

    Library_Init();

    // Determine our basedir and other global paths.
    determineGlobalPaths(&app);

    if(!DD_EarlyInit())
    {
        Sys_MessageBox(MBT_ERROR, DOOMSDAY_NICENAME, "Error during early init.", 0);
    }
    else if(!initDGL())
    {
        Sys_MessageBox(MBT_ERROR, DOOMSDAY_NICENAME, "Error initializing DGL.", 0);
    }
    else
    {   // All initialization complete.
        failed = FALSE;
    }

#ifdef __CLIENT__
    // No Windows system keys?
    if(CommandLine_Check("-nowsk"))
    {
        // Disable Alt-Tab, Alt-Esc, Ctrl-Alt-Del.  A bit of a hack...
        SystemParametersInfo(SPI_SETSCREENSAVERRUNNING, TRUE, 0, 0);
        Con_Message("Windows system keys disabled.");
    }
#endif

    return !failed;
}

/**
 * Shuts down the engine.
 */
void DD_Shutdown()
{
    DD_ShutdownAll(); // Stop all engine subsystems.
    Plug_UnloadAll();
    Library_Shutdown();

#ifdef UNICODE
    M_Free(convBuf); convBuf = 0;
    M_Free(utf8ConvBuf); utf8ConvBuf = 0;
#endif

    // No more use of COM beyond, this point.
    CoUninitialize();

#ifdef __CLIENT__
    DisplayMode_Shutdown();
#endif
}

/**
 * Windows implementation for the Unix strcasestr() function.
 */
const char* strcasestr(const char *text, const char *sub)
{
    int textLen = strlen(text);
    int subLen = strlen(sub);
    int i;

    for(i = 0; i <= textLen - subLen; ++i)
    {
        const char* start = text + i;
        if(!strnicmp(start, sub, subLen)) return start;
    }
    return 0;
}
