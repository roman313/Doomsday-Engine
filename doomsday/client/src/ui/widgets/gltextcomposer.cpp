/** @file gltextcomposer.cpp
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

#include "ui/widgets/gltextcomposer.h"

#include <QList>

using namespace de;

DENG2_PIMPL(GLTextComposer)
{
    Font const *font;
    Atlas *atlas;
    String text;
    FontLineWrapping const *wraps;
    bool needRaster;

    struct Line {
        Id id;
        shell::Range range;

        Line() : id(Id::None) {}
    };
    typedef QList<Line> Lines;
    Lines lines;

    Instance(Public *i) : Base(i), font(0), atlas(0), wraps(0), needRaster(false)
    {}

    ~Instance()
    {
        releaseLines();
    }

    void releaseLines()
    {
        DENG2_ASSERT(atlas != 0);

        foreach(Line const &line, lines)
        {
            atlas->release(line.id);
        }
        lines.clear();
    }

    bool allocLines()
    {
        bool changed = false;

        for(int i = 0; i < wraps->height(); ++i)
        {
            shell::WrappedLine const span = wraps->line(i);

            if(i < lines.size())
            {
                if(lines[i].range == span.range)
                {
                    // This can be kept as is.
                    continue;
                }

                // Needs to be redone.
                atlas->release(lines[i].id);
            }

            changed = true;

            String const part = text.substr(span.range.start, span.range.size());

            if(i >= lines.size())
            {
                // Need another line.
                lines.append(Line());
            }

            Line &line = lines[i];
            line.range = span.range;
            line.id = atlas->alloc(font->rasterize(part));
        }

        // Remove the excess lines.
        while(lines.size() > wraps->height())
        {
            atlas->release(lines.back().id);
            lines.takeLast();
            changed = true;
        }

        DENG2_ASSERT(wraps->height() == lines.size());

        return changed;
    }
};

GLTextComposer::GLTextComposer() : d(new Instance(this))
{}

void GLTextComposer::release()
{
    d->releaseLines();
}

void GLTextComposer::setAtlas(Atlas &atlas)
{
    d->atlas = &atlas;
}

void GLTextComposer::setWrapping(FontLineWrapping const &wrappedLines)
{
    d->wraps = &wrappedLines;
    d->font = &d->wraps->font();
}

void GLTextComposer::setText(String const &text)
{
    d->text = text;
    d->needRaster = true; // Force a redo of everything.
}

bool GLTextComposer::update()
{
    if(d->needRaster)
    {
        d->releaseLines();
        d->needRaster = false;
    }
    return d->allocLines();
}

void GLTextComposer::makeVertices(Vertices &triStrip,
                                  Vector2i const &topLeft,
                                  Alignment const &lineAlign)
{
    makeVertices(triStrip, Rectanglei(topLeft, topLeft), AlignTop | AlignLeft, lineAlign);
}

void GLTextComposer::makeVertices(Vertices &triStrip,
                                  Rectanglei const &rect,
                                  Alignment const &alignInRect,
                                  Alignment const &lineAlign)
{
    // Top left corner.
    Vector2f p = rect.topLeft;

    Vector2i const contentSize(d->wraps->width(), d->wraps->totalHeightInPixels());

    // Apply alignment within the provided rectangle.
    if(alignInRect.testFlag(AlignRight))
    {
        p.x += int(rect.width()) - contentSize.x;
    }
    else if(!alignInRect.testFlag(AlignLeft))
    {
        p.x += (int(rect.width()) - contentSize.x) / 2;
    }
    if(alignInRect.testFlag(AlignBottom))
    {
        p.y += int(rect.height()) - contentSize.y;
    }
    else if(!alignInRect.testFlag(AlignTop))
    {
        p.y += (int(rect.height()) - contentSize.y) / 2;
    }

    DENG2_ASSERT(d->wraps->height() == d->lines.size());

    // Generate vertices for each line.
    for(int i = 0; i < d->wraps->height(); ++i)
    {
        // Empty lines are skipped.
        if(!d->lines[i].id.isNone())
        {
            Vector2ui const size = d->atlas->imageRect(d->lines[i].id).size();
            Rectanglef const uv  = d->atlas->imageRectf(d->lines[i].id);

            Vertex v;
            Vertices quad;

            v.rgba = Vector4f(1, 1, 1, 1); // should be a param

            Vector2f linePos = p;

            // Align the line.
            if(lineAlign.testFlag(AlignRight))
            {
                linePos.x += int(rect.width()) - int(size.x);
            }
            else if(!lineAlign.testFlag(AlignLeft))
            {
                linePos.x += (int(rect.width()) - int(size.x)) / 2;
            }

            v.pos = linePos;                            v.texCoord = uv.topLeft;      quad << v;
            v.pos = linePos + Vector2f(size.x, 0);      v.texCoord = uv.topRight();   quad << v;
            v.pos = linePos + Vector2f(0, size.y);      v.texCoord = uv.bottomLeft(); quad << v;
            v.pos = linePos + Vector2f(size.x, size.y); v.texCoord = uv.bottomRight;  quad << v;

            VertexBuf::concatenate(quad, triStrip);
        }
        p.y += d->font->lineSpacing().value();
    }
}