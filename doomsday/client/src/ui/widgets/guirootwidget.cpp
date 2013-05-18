/** @file guirootwidget.cpp  Graphical root widget.
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

#include "ui/widgets/guirootwidget.h"
#include "ui/clientwindow.h"
#include "clientapp.h"

#include <de/AtlasTexture>
#include <de/GLTexture>

using namespace de;

DENG2_PIMPL(GuiRootWidget)
{
    ClientWindow *window;
    QScopedPointer<AtlasTexture> atlas; ///< Shared atlas for most UI graphics/text.

    Instance(Public *i, ClientWindow *win)
        : Base(i),
          window(win),
          atlas(0)
    {}
};

GuiRootWidget::GuiRootWidget(ClientWindow *window)
    : d(new Instance(this, window))
{}

void GuiRootWidget::setWindow(ClientWindow *window)
{
    d->window = window;
}

ClientWindow &GuiRootWidget::window()
{
    DENG2_ASSERT(d->window != 0);
    return *d->window;
}

AtlasTexture &GuiRootWidget::atlas()
{
    if(d->atlas.isNull())
    {
        d->atlas.reset(AtlasTexture::newWithRowAllocator(
                           Atlas::BackingStore | Atlas::AllowDefragment,
                           GLTexture::maximumSize()));
    }
    return *d->atlas;
}

GLShaderBank &GuiRootWidget::shaders()
{
    return ClientApp::glShaderBank();
}

void GuiRootWidget::update()
{
    // Allow GL operations.
    window().canvas().makeCurrent();

    RootWidget::update();
}