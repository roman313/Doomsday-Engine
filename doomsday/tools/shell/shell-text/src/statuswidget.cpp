/** @file statuswidget.cpp  Widget for status information.
 *
 * @authors Copyright © 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#include "statuswidget.h"
#include <QTimer>
#include <de/shell/TextRootWidget>

using namespace de;
using namespace de::shell;

struct StatusWidget::Instance
{
    StatusWidget &self;
    Link *link;
    QTimer *updateTimer;

    Instance(StatusWidget &i) : self(i), link(0)
    {
        updateTimer = new QTimer(&self);
    }
};

StatusWidget::StatusWidget(String const &name)
    : TextWidget(name), d(new Instance(*this))
{
    connect(d->updateTimer, SIGNAL(timeout()), this, SLOT(redraw()));
}

StatusWidget::~StatusWidget()
{
    delete d;
}

void StatusWidget::setShellLink(Link *link)
{
    d->link = link;

    if(link)
    {
        // Observe changes in link status.
        connect(link, SIGNAL(connected()), this, SLOT(linkConnected()));
        connect(link, SIGNAL(disconnected()), this, SLOT(linkDisconnected()));
    }

    root().requestDraw();
}

void StatusWidget::draw()
{
    if(!targetCanvas()) return;

    Rectanglei pos = rule().recti();
    TextCanvas buf(pos.size());

    if(!d->link || d->link->status() == Link::Disconnected)
    {
        String msg = "Not connected to a server";
        buf.drawText(Vector2i(buf.size().x/2 - msg.size()/2), msg, TextCanvas::Char::Bold);
    }
    else if(d->link->status() == Link::Connecting)
    {
        String msg = "Connecting to " + d->link->address().asText();
        buf.drawText(Vector2i(buf.size().x/2 - msg.size()/2), msg);
    }
    else if(d->link->status() == Link::Connected)
    {
        TimeDelta elapsed = d->link->connectedAt().since();
        String time = String("| %1:%2:%3")
                .arg(int(elapsed.asHours()))
                .arg(int(elapsed.asMinutes()) % 60)
                .arg(int(elapsed) % 60);
        String host = "| " + d->link->address().asText();

        int x = buf.size().x - time.size() - 1;
        buf.drawText(x, time);

        x -= host.size() + 1;
        buf.drawText(x, host);
    }

    buf.blit(*targetCanvas(), pos.topLeft);
}

void StatusWidget::redraw()
{
    drawAndShow();
}

void StatusWidget::linkConnected()
{
    d->updateTimer->start(1000);
    redraw();
}

void StatusWidget::linkDisconnected()
{
    d->updateTimer->stop();
    redraw();
}
