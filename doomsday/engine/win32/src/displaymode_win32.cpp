/**
 * @file displaymode_win32.cpp
 * Win32 implementation of the DisplayMode native functionality. @ingroup gl
 *
 * @authors Copyright © 2003-2012 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2005-2012 Daniel Swanson <danij@dengine.net>
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
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#include <qDebug>

#include "de_platform.h"
//#include <windowsx.h>

#include "displaymode_native.h"

#include <assert.h>
#include <vector>

static std::vector<DEVMODE> devModes;
static DEVMODE currentDevMode;

static DisplayMode devToDisplayMode(const DEVMODE& d)
{
    DisplayMode m;
    m.width = d.dmPelsWidth;
    m.height = d.dmPelsHeight;
    m.depth = d.dmBitsPerPel;
    m.refreshRate = d.dmDisplayFrequency;
    return m;
}

void DisplayMode_Native_Init(void)
{
    // Let's see which modes are available.
    for(int i = 0; ; i++)
    {
        DEVMODE mode;
        memset(&mode, 0, sizeof(mode));
        mode.dmSize = sizeof(mode);
        if(!EnumDisplaySettings(NULL, i, &mode))
            break; // That's all.

        devModes.push_back(mode);
    }

    // And which is the current mode?
    memset(&currentDevMode, 0, sizeof(currentDevMode));
    currentDevMode.dmSize = sizeof(currentDevMode);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &currentDevMode);
}

void DisplayMode_Native_Shutdown(void)
{
    devModes.clear();
}

int DisplayMode_Native_Count(void)
{
    return devModes.size();
}

void DisplayMode_Native_GetMode(int index, DisplayMode* mode)
{
    assert(index >= 0 && index < DisplayMode_Native_Count());
    *mode = devToDisplayMode(devModes[index]);
}

void DisplayMode_Native_GetCurrentMode(DisplayMode* mode)
{
    *mode = devToDisplayMode(currentDevMode);
}

static int findMode(const DisplayMode* mode)
{
    for(int i = 0; i < DisplayMode_Native_Count(); ++i)
    {
        DisplayMode d = devToDisplayMode(devModes[i]);
        if(DisplayMode_IsEqual(&d, mode))
        {
            return i;
        }
    }
    return -1;
}

int DisplayMode_Native_Change(const DisplayMode* mode, boolean shouldCapture)
{
    assert(mode);
    assert(findMode(mode) >= 0);

    DEVMODE m = devModes[findMode(mode)];
    m.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

    if(ChangeDisplaySettings(&m, shouldCapture? CDS_FULLSCREEN : 0) != DISP_CHANGE_SUCCESSFUL)
        return false;

    currentDevMode = m;
    return true;
}