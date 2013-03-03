/** @file windowsystem.cpp  Window management subsystem.
 *
 * @authors Copyright (c) 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#include "de_platform.h"
#include "ui/windowsystem.h"
#include "ui/window.h"

using namespace de;

DENG2_PIMPL(WindowSystem)
{
    /// @todo Creation and ownership of windows belongs here (from window.cpp).

    Instance(Public *i) : Base(i)
    {}
};

WindowSystem::WindowSystem()
    : System(ObservesTime | ReceivesInputEvents), d(new Instance(this))
{}

WindowSystem::~WindowSystem()
{
    delete d;
}

bool WindowSystem::processEvent(Event const &event)
{
    CanvasWindow *win = Window_CanvasWindow(Window_Main());
    DENG2_ASSERT(win != 0);

    return win->root().processEvent(event);
}

void WindowSystem::timeChanged(Clock const &/*clock*/)
{
    CanvasWindow *win = Window_CanvasWindow(Window_Main());
    DENG2_ASSERT(win != 0);

    win->root().update();
}