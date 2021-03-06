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
#include "ui/clientwindow.h"
#include "ui/style.h"
#include "clientapp.h"

#include <QMap>

using namespace de;

DENG2_PIMPL(WindowSystem)
{
    typedef QMap<String, ClientWindow *> Windows;
    Windows windows;
    Style style;

    // Mouse motion.
    bool mouseMoved;
    Vector2i latestMousePos;

    Instance(Public *i) : Base(i), mouseMoved(false)
    {
        style.load(App::fileSystem().find("defaultstyle.pack").path());
    }

    ~Instance()
    {
        self.closeAll();
    }

    void dispatchLatestMousePosition()
    {
        if(mouseMoved)
        {
            mouseMoved = false;

            self.main().root().processEvent(MouseEvent(MouseEvent::Absolute, latestMousePos));
        }
    }
};

WindowSystem::WindowSystem()
    : System(ObservesTime | ReceivesInputEvents), d(new Instance(this))
{
    ClientWindow::setDefaultGLFormat();
}

ClientWindow *WindowSystem::createWindow(String const &id)
{
    DENG2_ASSERT(!d->windows.contains(id));

    ClientWindow *win = new ClientWindow(id);
    d->windows.insert(id, win);
    return win;
}

bool WindowSystem::hasMain() // static
{
    return ClientApp::windowSystem().d->windows.contains("main");
}

ClientWindow &WindowSystem::main() // static
{
    return static_cast<ClientWindow &>(PersistentCanvasWindow::main());
}

ClientWindow *WindowSystem::find(String const &id) const
{
    Instance::Windows::const_iterator found = d->windows.constFind(id);
    if(found != d->windows.constEnd())
    {
        return found.value();
    }
    return 0;
}

void WindowSystem::closeAll()
{
    qDeleteAll(d->windows.values());
    d->windows.clear();
}

Style &WindowSystem::style()
{
    return d->style;
}

bool WindowSystem::processEvent(Event const &event)
{
    /*
     * Mouse motion is filtered as it may be produced needlessly often with
     * high-frequency mice. Note that this does not affect DirectInput mouse
     * input at all (however, as DirectInput is polled once per frame, it is
     * already filtered anyway).
     */

    if(event.type() == Event::MousePosition)
    {
        MouseEvent const &mouse = event.as<MouseEvent>();

        if(mouse.pos() != d->latestMousePos)
        {
            // This event will be emitted later, before widget tree update.
            d->latestMousePos = mouse.pos();
            d->mouseMoved = true;
        }
        return true;
    }

    // Dispatch the event to the main window's widget tree.
    return main().root().processEvent(event);
}

void WindowSystem::timeChanged(Clock const &/*clock*/)
{
    d->dispatchLatestMousePosition();

    main().root().update();
}
