/** @file notificationwidget.cpp  Notification area.
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

#include "ui/widgets/notificationwidget.h"
#include "ui/widgets/guirootwidget.h"

#include <de/Drawable>
#include <de/Matrix>
#include <de/ScalarRule>

#include <QMap>
#include <QTimer>

using namespace de;

static TimeDelta const ANIM_SPAN = .5;

DENG2_PIMPL(NotificationWidget)
{
    ScalarRule *shift;

    typedef QMap<GuiWidget *, Widget *> OldParents;
    OldParents oldParents;
    QTimer dismissTimer;
    QList<GuiWidget *> pendingDismiss;

    // GL objects:
    typedef DefaultVertexBuf VertexBuf;
    Drawable drawable;
    GLUniform uMvpMatrix;
    GLUniform uColor;

    Instance(Public *i)
        : Base(i),
          uMvpMatrix("uMvpMatrix", GLUniform::Mat4),
          uColor    ("uColor",     GLUniform::Vec4)
    {
        dismissTimer.setSingleShot(true);
        dismissTimer.setInterval(ANIM_SPAN.asMilliSeconds());
        QObject::connect(&dismissTimer, SIGNAL(timeout()), thisPublic, SLOT(dismiss()));

        shift = new ScalarRule(0);
        updateStyle();
    }

    ~Instance()
    {
        releaseRef(shift);
    }

    void updateStyle()
    {
        self.set(Background(self.style().colors().colorf("background")));
    }

    void glInit()
    {
        drawable.addBuffer(new VertexBuf);

        self.root().shaders().build(drawable.program(), "generic.color_ucolor")
                << uMvpMatrix << uColor;
    }

    void glDeinit()
    {
        drawable.clear();
    }

    void updateGeometry()
    {
        Rectanglei pos;
        if(self.hasChangedPlace(pos) || self.geometryRequested())
        {
            self.requestGeometry(false);

            VertexBuf::Builder verts;
            self.glMakeGeometry(verts);
            drawable.buffer<VertexBuf>().setVertices(gl::TriangleStrip, verts, gl::Static);
        }
    }

    void updateChildLayout()
    {
        Rule const &gap = self.style().rules().rule("unit");

        Rule const *totalWidth = 0;
        Rule const *totalHeight = 0;

        WidgetList const children = self.Widget::children();
        for(int i = 0; i < children.size(); ++i)
        {
            GuiWidget &w = children[i]->as<GuiWidget>();

            // The children are laid out simply in a row from right to left.
            w.rule().setInput(Rule::Top, self.rule().top());
            if(i > 0)
            {
                w.rule().setInput(Rule::Right, children[i - 1]->as<GuiWidget>().rule().left() - gap);
                changeRef(totalWidth, *totalWidth + gap + w.rule().width());
            }
            else
            {
                w.rule().setInput(Rule::Right, self.rule().right());
                totalWidth = holdRef(w.rule().width());
            }

            if(!totalHeight)
            {
                totalHeight = holdRef(w.rule().height());
            }
            else
            {
                changeRef(totalHeight, OperatorRule::maximum(*totalHeight, w.rule().height()));
            }
        }

        // Update the total size of the notification area.
        self.rule()
                .setInput(Rule::Width,  *totalWidth)
                .setInput(Rule::Height, *totalHeight);

        releaseRef(totalWidth);
        releaseRef(totalHeight);
    }

    void show()
    {
        //self.setOpacity(1, ANIM_SPAN);
        shift->set(0, ANIM_SPAN);
        shift->setStyle(Animation::EaseOut);
    }

    void hide(TimeDelta const &span = ANIM_SPAN)
    {
        //self.setOpacity(0, span);
        shift->set(self.rule().height() + self.style().rules().rule("gap"), span);
        shift->setStyle(Animation::EaseIn);
    }

    void dismissChild(GuiWidget &notif)
    {
        notif.hide();
        self.remove(notif);

        if(oldParents.contains(&notif))
        {
            oldParents[&notif]->add(&notif);
            oldParents.remove(&notif);
        }
    }

    void performPendingDismiss()
    {
        dismissTimer.stop();

        // The pending children were already asked to be dismissed.
        foreach(GuiWidget *w, pendingDismiss)
        {
            dismissChild(*w);
        }
        pendingDismiss.clear();
    }
};

NotificationWidget::NotificationWidget(String const &name) : d(new Instance(this))
{
    // Initially the widget is empty.
    rule().setSize(Const(0), Const(0));
    d->shift->set(style().fonts().font("default").height().valuei() +
                  style().rules().rule("gap").valuei() * 3);
    hide();
}

Rule const &NotificationWidget::shift()
{
    return *d->shift;
}

void NotificationWidget::showChild(GuiWidget *notif)
{
    DENG2_ASSERT(notif != 0);

    if(isChildShown(*notif))
    {
        // Already in the notification area.
        return;
    }

    // Cancel a pending dismissal.
    d->performPendingDismiss();

    if(notif->parentWidget())
    {
        d->oldParents.insert(notif, notif->parentWidget());
        /// @todo Should observe if the old parent is destroyed.
    }
    add(notif);
    notif->show();
    d->show();
}

void NotificationWidget::hideChild(GuiWidget &notif)
{
    if(!isChildShown(notif))
    {
        // Already in the notification area.
        return;
    }

    if(childCount() > 1)
    {
        // Dismiss immediately, the area itself remains open.
        d->dismissChild(notif);
    }
    else
    {
        // The last one should be deferred until the notification area
        // itself is dismissed.
        d->dismissTimer.start();
        d->pendingDismiss << &notif;
        d->hide();
    }
}

void NotificationWidget::dismiss()
{
    d->performPendingDismiss();
}

bool NotificationWidget::isChildShown(GuiWidget &notif) const
{
    if(d->pendingDismiss.contains(&notif))
    {
        return false;
    }
    return notif.parentWidget() == this;
}

void NotificationWidget::viewResized()
{
    d->uMvpMatrix = root().projMatrix2D();
}

void NotificationWidget::drawContent()
{
    d->updateGeometry();

    d->uColor = Vector4f(1, 1, 1, visibleOpacity());
    d->drawable.draw();
}

void NotificationWidget::glInit()
{
    d->glInit();
}

void NotificationWidget::glDeinit()
{
    d->glDeinit();
}

void NotificationWidget::addedChildWidget(GuiWidget &)
{
    d->updateChildLayout();
    show();
}

void NotificationWidget::removedChildWidget(GuiWidget &)
{
    d->updateChildLayout();
    if(!childCount())
    {
        hide();
    }
}
