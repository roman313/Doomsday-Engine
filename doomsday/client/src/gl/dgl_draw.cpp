/** @file dgl_draw.cpp
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2007-2013 Daniel Swanson <danij@dengine.net>
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

/**
 * Drawing Operations and Vertex Arrays.
 *
 * Get OpenGL header files from:
 * http://oss.sgi.com/projects/ogl-sample/
 */

// HEADER FILES ------------------------------------------------------------

#define DENG_NO_API_MACROS_GL

#include <stdio.h>
#include <stdlib.h>

#include "de_base.h"
#include "de_console.h"
#include "de_graphics.h"
#include "de_misc.h"

#include "gl/sys_opengl.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// FUNCTION PROTOTYPES -----------------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int primLevel = 0;
static DGLuint inList = 0;
#ifdef _DEBUG
static boolean inPrim = false;
#endif

// CODE --------------------------------------------------------------------

boolean GL_NewList(DGLuint list, int mode)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    // We enter a New/End list section.
#ifdef _DEBUG
if(inList)
    Con_Error("GL_NewList: Already in list");
Sys_GLCheckError();
#endif

    if(list)
    {   // A specific list id was requested. Is it free?
        if(glIsList(list))
        {
#if _DEBUG
Con_Error("GL_NewList: List %u already in use.", (unsigned int) list);
#endif
            return false;
        }
    }
    else
    {
        // Just get a new list id, it doesn't matter.
        list = glGenLists(1);
    }

    glNewList(list, mode == DGL_COMPILE? GL_COMPILE : GL_COMPILE_AND_EXECUTE);
    inList = list;
    return true;
}

DGLuint GL_EndList(void)
{
    DGLuint currentList = inList;

    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glEndList();
#ifdef _DEBUG
    inList = 0;
    Sys_GLCheckError();
#endif

    return currentList;
}

void GL_CallList(DGLuint list)
{
    if(!list) return; // We do not consider zero a valid list id.

    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glCallList(list);
}

void GL_DeleteLists(DGLuint list, int range)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glDeleteLists(list, range);
}

DENG_EXTERN_C void DGL_Color3ub(DGLubyte r, DGLubyte g, DGLubyte b)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor3ub(r, g, b);
}

DENG_EXTERN_C void DGL_Color3ubv(const DGLubyte* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor3ubv(vec);
}

DENG_EXTERN_C void DGL_Color4ub(DGLubyte r, DGLubyte g, DGLubyte b, DGLubyte a)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor4ub(r, g, b, a);
}

DENG_EXTERN_C void DGL_Color4ubv(const DGLubyte* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor4ubv(vec);
}

DENG_EXTERN_C void DGL_Color3f(float r, float g, float b)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor3f(r, g, b);
}

DENG_EXTERN_C void DGL_Color3fv(const float* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor3fv(vec);
}

DENG_EXTERN_C void DGL_Color4f(float r, float g, float b, float a)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor4f(r, g, b, a);
}

DENG_EXTERN_C void DGL_Color4fv(const float* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor4fv(vec);
}

DENG_EXTERN_C void DGL_TexCoord2f(byte target, float s, float t)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glMultiTexCoord2f(GL_TEXTURE0 + target, s, t);
}

DENG_EXTERN_C void DGL_TexCoord2fv(byte target, float* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glMultiTexCoord2fv(GL_TEXTURE0 + target, vec);
}

DENG_EXTERN_C void DGL_Vertex2f(float x, float y)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glVertex2f(x, y);
}

DENG_EXTERN_C void DGL_Vertex2fv(const float* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glVertex2fv(vec);
}

DENG_EXTERN_C void DGL_Vertex3f(float x, float y, float z)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glVertex3f(x, y, z);
}

DENG_EXTERN_C void DGL_Vertex3fv(const float* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glVertex3fv(vec);
}

DENG_EXTERN_C void DGL_Vertices2ftv(int num, const dgl_ft2vertex_t* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    for(; num > 0; num--, vec++)
    {
        glTexCoord2fv(vec->tex);
        glVertex2fv(vec->pos);
    }
}

DENG_EXTERN_C void DGL_Vertices3ftv(int num, const dgl_ft3vertex_t* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    for(; num > 0; num--, vec++)
    {
        glTexCoord2fv(vec->tex);
        glVertex3fv(vec->pos);
    }
}

DENG_EXTERN_C void DGL_Vertices3fctv(int num, const dgl_fct3vertex_t* vec)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    for(; num > 0; num--, vec++)
    {
        glColor4fv(vec->color);
        glTexCoord2fv(vec->tex);
        glVertex3fv(vec->pos);
    }
}

DENG_EXTERN_C void DGL_Begin(dglprimtype_t mode)
{
    if(novideo)
        return;

    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    // We enter a Begin/End section.
    primLevel++;

#ifdef _DEBUG
    if(inPrim)
        Con_Error("OpenGL: already inPrim");
    inPrim = true;
    Sys_GLCheckError();
#endif

    glBegin(mode == DGL_POINTS ? GL_POINTS : mode ==
            DGL_LINES ? GL_LINES : mode ==
            DGL_TRIANGLES ? GL_TRIANGLES : mode ==
            DGL_TRIANGLE_FAN ? GL_TRIANGLE_FAN : mode ==
            DGL_TRIANGLE_STRIP ? GL_TRIANGLE_STRIP : mode ==
            DGL_QUAD_STRIP ? GL_QUAD_STRIP : GL_QUADS);
}

DENG_EXTERN_C void DGL_End(void)
{
    if(novideo)
        return;

    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    if(primLevel > 0)
    {
        primLevel--;
        glEnd();
    }

#ifdef _DEBUG
    inPrim = false;
    Sys_GLCheckError();
#endif
}

DENG_EXTERN_C boolean DGL_NewList(DGLuint list, int mode)
{
    return GL_NewList(list, mode);
}

DENG_EXTERN_C DGLuint DGL_EndList(void)
{
    return GL_EndList();
}

DENG_EXTERN_C void DGL_CallList(DGLuint list)
{
    GL_CallList(list);
}

DENG_EXTERN_C void DGL_DeleteLists(DGLuint list, int range)
{
    GL_DeleteLists(list, range);
}

DENG_EXTERN_C void DGL_DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
    GL_DrawLine(x1, y1, x2, y2, r, g, b, a);
}

DENG_EXTERN_C void DGL_DrawRect(const RectRaw* rect)
{
    GL_DrawRect(rect);
}

DENG_EXTERN_C void DGL_DrawRect2(int x, int y, int w, int h)
{
    GL_DrawRect2(x, y, w, h);
}

DENG_EXTERN_C void DGL_DrawRectf(const RectRawf* rect)
{
    GL_DrawRectf(rect);
}

DENG_EXTERN_C void DGL_DrawRectf2(double x, double y, double w, double h)
{
    GL_DrawRectf2(x, y, w, h);
}

DENG_EXTERN_C void DGL_DrawRectf2Color(double x, double y, double w, double h, float r, float g, float b, float a)
{
    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    glColor4f(r, g, b, a);
    GL_DrawRectf2(x, y, w, h);
}

DENG_EXTERN_C void DGL_DrawRectf2Tiled(double x, double y, double w, double h, int tw, int th)
{
    GL_DrawRectf2Tiled(x, y, w, h, tw, th);
}

DENG_EXTERN_C void DGL_DrawCutRectfTiled(const RectRawf* rect, int tw, int th, int txoff, int tyoff,
    const RectRawf* cutRect)
{
    GL_DrawCutRectfTiled(rect, tw, th, txoff, tyoff, cutRect);
}

DENG_EXTERN_C void DGL_DrawCutRectf2Tiled(double x, double y, double w, double h, int tw, int th,
    int txoff, int tyoff, double cx, double cy, double cw, double ch)
{
    GL_DrawCutRectf2Tiled(x, y, w, h, tw, th, txoff, tyoff, cx, cy, cw, ch);
}

DENG_EXTERN_C void DGL_DrawQuadOutline(const Point2Raw* tl, const Point2Raw* tr, const Point2Raw* br,
    const Point2Raw* bl, const float color[4])
{
    if(!tl || !tr || !br || !bl || (color && !(color[CA] > 0))) return;

    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    if(color) DGL_Color4fv(color);
    glBegin(GL_LINE_LOOP);
        glVertex2iv((const GLint*)tl->xy);
        glVertex2iv((const GLint*)tr->xy);
        glVertex2iv((const GLint*)br->xy);
        glVertex2iv((const GLint*)bl->xy);
    glEnd();
}

DENG_EXTERN_C void DGL_DrawQuad2Outline(int tlX, int tlY, int trX, int trY, int brX, int brY, int blX,
    int blY, const float color[4])
{
    Point2Raw tl, tr, bl, br;
    tl.x = tlX;
    tl.y = tlY;
    tr.x = trX;
    tr.y = trY;
    br.x = brX;
    br.y = brY;
    bl.x = blX;
    bl.y = blY;
    DGL_DrawQuadOutline(&tl, &tr, &br, &bl, color);
}
