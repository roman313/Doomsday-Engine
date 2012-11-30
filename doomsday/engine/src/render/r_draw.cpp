/**
 * @file r_draw.cpp Misc Drawing Routines
 * @ingroup render
 *
 * @author Copyright &copy; 2003-2012 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @author Copyright &copy; 2006-2012 Daniel Swanson <danij@dengine.net>
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

#include "de_base.h"
#include "de_console.h"
#include "de_graphics.h"
#include "de_play.h"

#include "render/r_draw.h"
#include "gl/sys_opengl.h"
#include "resource/texture.h"
#include "resource/texturevariant.h"
#include "resource/materialvariant.h"

// A logical ordering (twice around).
enum {
    BG_BACKGROUND,
    BG_TOP,
    BG_RIGHT,
    BG_BOTTOM,
    BG_LEFT,
    BG_TOPLEFT,
    BG_TOPRIGHT,
    BG_BOTTOMRIGHT,
    BG_BOTTOMLEFT
};

static bool inited = false;
static int borderSize;
static Uri *borderGraphicsNames[9];
static patchid_t borderPatches[9];

static void loadViewBorderPatches()
{
    borderPatches[0] = 0;
    for(uint i = 1; i < 9; ++i)
    {
        borderPatches[i] = R_DeclarePatch(Str_Text(Uri_Path(borderGraphicsNames[i])));
    }

    // Detemine the view border size.
    borderSize = 0;
    patchinfo_t info;
    if(!R_GetPatchInfo(borderPatches[BG_TOP], &info)) return;
    borderSize = info.geometry.size.height;
}

void R_SetBorderGfx(Uri const *const *paths)
{
    DENG_ASSERT(inited);
    if(!paths) Con_Error("R_SetBorderGfx: Missing argument.");

    for(uint i = 0; i < 9; ++i)
    {
        if(paths[i])
        {
            if(!borderGraphicsNames[i])
                borderGraphicsNames[i] = Uri_Dup(paths[i]);
            else
                Uri_Copy(borderGraphicsNames[i], paths[i]);
        }
        else
        {
            if(borderGraphicsNames[i])
                Uri_Delete(borderGraphicsNames[i]);
            borderGraphicsNames[i] = 0;
        }
    }

    loadViewBorderPatches();
}

void R_InitViewWindow(void)
{
    for(int i = 0; i < DDMAXPLAYERS; ++i)
    {
        R_SetupDefaultViewWindow(i);
    }

    if(inited)
    {
        for(int i = 0; i < 9; ++i)
        {
            if(borderGraphicsNames[i])
            {
                Uri_Delete(borderGraphicsNames[i]);
            }
        }
    }
    memset(borderGraphicsNames, 0, sizeof(borderGraphicsNames));
    memset(borderPatches, 0, sizeof(borderPatches));
    borderSize = 0;
    inited = true;
}

void R_ShutdownViewWindow(void)
{
    if(!inited) return;

    for(uint i = 0; i < 9; ++i)
    {
        if(borderGraphicsNames[i])
            Uri_Delete(borderGraphicsNames[i]);
    }

    memset(borderGraphicsNames, 0, sizeof(borderGraphicsNames));
    inited = false;
}

void R_DrawPatch3(Texture *tex, int x, int y, int w, int h, boolean useOffsets)
{
    if(!tex) return;

    if(Textures_Scheme(Textures_Id(tex)) != TS_PATCHES)
    {
#if _DEBUG
        LOG_AS("R_DrawPatch3");
        LOG_WARNING("Attempted to draw a non-patch [%p].") << de::dintptr(tex);
#endif
        return;
    }

    GL_BindTexture(GL_PreparePatchTexture(tex));
    if(useOffsets)
    {
        patchtex_t *pTex = reinterpret_cast<patchtex_t *>(Texture_UserDataPointer(tex));
        DENG_ASSERT(pTex);

        x += pTex->offX;
        y += pTex->offY;
    }

    GL_DrawRectf2Color(x, y, w, h, 1, 1, 1, 1);
}

void R_DrawPatch2(Texture *tex, int x, int y, int w, int h)
{
    R_DrawPatch3(tex, x, y, w, h, true);
}

void R_DrawPatch(Texture *tex, int x, int y)
{
    R_DrawPatch2(tex, x, y, Texture_Width(tex), Texture_Height(tex));
}

void R_DrawPatchTiled(Texture *tex, int x, int y, int w, int h, int wrapS, int wrapT)
{
    if(!tex) return;

    GL_BindTexture(GL_PreparePatchTexture2(tex, wrapS, wrapT));
    GL_DrawRectf2Tiled(x, y, w, h, Texture_Width(tex), Texture_Height(tex));
}

materialvariantspecification_t const *Ui_MaterialSpec()
{
    return Materials_VariantSpecificationForContext(MC_UI, 0, 0, 0, 0, GL_REPEAT, GL_REPEAT,
                                                    0, -3, 0, false, false, false, false);
}

void R_DrawViewBorder(void)
{
    DENG_ASSERT(inited);

    viewport_t const *port = R_CurrentViewPort();
    viewdata_t const *vd = R_ViewData(displayPlayer);
    DENG_ASSERT(port && vd);

    if(0 == vd->window.size.width || 0 == vd->window.size.height) return;
    if(vd->window.size.width == port->geometry.size.width && vd->window.size.height == port->geometry.size.height) return;

    LIBDENG_ASSERT_IN_MAIN_THREAD();
    LIBDENG_ASSERT_GL_CONTEXT_ACTIVE();

    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();

    // Scale from viewport space to fixed 320x200 space.
    int border;
    if(port->geometry.size.width >= port->geometry.size.height)
    {
        glScalef(float(SCREENHEIGHT) / port->geometry.size.height, float(SCREENHEIGHT) / port->geometry.size.height, 1);
        border = float(borderSize) / SCREENHEIGHT * port->geometry.size.height;
    }
    else
    {
        glScalef(float(SCREENWIDTH) / port->geometry.size.width, float(SCREENWIDTH) / port->geometry.size.width, 1);
        border = float(borderSize) / SCREENWIDTH * port->geometry.size.width;
    }

    glColor4f(1, 1, 1, 1);

    // View background.
    material_t *mat = Materials_ToMaterial(Materials_ResolveUri2(borderGraphicsNames[BG_BACKGROUND], true/*quiet please*/));
    if(mat)
    {
        materialsnapshot_t const *ms = Materials_Prepare(mat, Ui_MaterialSpec(), true);

        GL_BindTexture(MST(ms, MTU_PRIMARY));
        GL_DrawCutRectf2Tiled(0, 0, port->geometry.size.width, port->geometry.size.height, ms->size.width, ms->size.height, 0, 0,
                            vd->window.origin.x - border, vd->window.origin.y - border,
                            vd->window.size.width + 2 * border, vd->window.size.height + 2 * border);
    }

    if(border != 0)
    {
        R_DrawPatchTiled(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_TOP])), vd->window.origin.x, vd->window.origin.y - border, vd->window.size.width, border, GL_REPEAT, GL_CLAMP_TO_EDGE);
        R_DrawPatchTiled(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_BOTTOM])), vd->window.origin.x, vd->window.origin.y + vd->window.size.height , vd->window.size.width, border, GL_REPEAT, GL_CLAMP_TO_EDGE);
        R_DrawPatchTiled(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_LEFT])), vd->window.origin.x - border, vd->window.origin.y, border, vd->window.size.height, GL_CLAMP_TO_EDGE, GL_REPEAT);
        R_DrawPatchTiled(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_RIGHT])), vd->window.origin.x + vd->window.size.width, vd->window.origin.y, border, vd->window.size.height, GL_CLAMP_TO_EDGE, GL_REPEAT);
    }

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    if(border != 0)
    {
        R_DrawPatch3(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_TOPLEFT])), vd->window.origin.x - border, vd->window.origin.y - border, border, border, false);
        R_DrawPatch3(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_TOPRIGHT])), vd->window.origin.x + vd->window.size.width, vd->window.origin.y - border, border, border, false);
        R_DrawPatch3(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_BOTTOMRIGHT])), vd->window.origin.x + vd->window.size.width, vd->window.origin.y + vd->window.size.height, border, border, false);
        R_DrawPatch3(Textures_ToTexture(Textures_TextureForUniqueId(TS_PATCHES, borderPatches[BG_BOTTOMLEFT])), vd->window.origin.x - border, vd->window.origin.y + vd->window.size.height, border, border, false);
    }

    glDisable(GL_TEXTURE_2D);
}