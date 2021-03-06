/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2006 Jaakko Keränen <skyjake@dengine.net>
 *\author Copyright © 2006 Daniel Swanson <danij@dengine.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/*
 * matrix.cpp: Matrix and Math Operations
 */

// HEADER FILES ------------------------------------------------------------

#include "drd3d.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

enum
{
    MAT_MODELVIEW,
    MAT_PROJECTION,
    MAT_TEXTURE,
    NUM_MATRIX_STACKS
};

// FUNCTION PROTOTYPES -----------------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

ID3DXMatrixStack    *matStack[NUM_MATRIX_STACKS];
int                 msIndex;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static D3DXMATRIX   identityMatrix;

// CODE --------------------------------------------------------------------

void InitMatrices(void)
{
    msIndex = MAT_MODELVIEW;
    // Create the stack objects.
    for(int i = 0; i < NUM_MATRIX_STACKS; ++i)
    {
        if(FAILED(hr = D3DXCreateMatrixStack(0, &matStack[i])))
        {
            DXError("D3DXCreateMatrixStack");
            return;
        }
        matStack[i]->LoadIdentity();
    }
    D3DXMatrixIdentity(&identityMatrix);
}

void ShutdownMatrices(void)
{
    for(int i = 0; i < NUM_MATRIX_STACKS; i++)
    {
        if(matStack[i]) matStack[i]->Release();
        matStack[i] = NULL;
    }
}

/**
 *  Updates the projection matrix.
 */
void ScissorProjection(void)
{
    D3DXMATRIX mat = *matStack[MAT_PROJECTION]->GetTop(), x, res;

    if(!scissorActive)
    {
        dev->SetTransform(D3DTS_PROJECTION, &mat);
        return;
    }

    // Calculate an additional translation and scaling to fit the scissor.
    D3DXMatrixMultiply(&res, &mat, D3DXMatrixTranslation(&x,
        (float) viewport.x - scissor.x,
        (float) viewport.y - scissor.y, 0));
    D3DXMatrixMultiply(&mat, &res, D3DXMatrixScaling(&x,
        viewport.width/(float)scissor.width,
        viewport.height/(float)scissor.height, 1));
    dev->SetTransform(D3DTS_PROJECTION, &mat);
}

void UploadMatrix(void)
{
    D3DTRANSFORMSTATETYPE d3dts[NUM_MATRIX_STACKS] =
    {
        D3DTS_VIEW, D3DTS_PROJECTION, D3DTS_TEXTURE0
    };

    if(msIndex == MAT_TEXTURE)
        return;

    if(msIndex == MAT_PROJECTION && scissorActive)
        ScissorProjection();
    else
        dev->SetTransform(d3dts[msIndex], matStack[msIndex]->GetTop());
}

/**
 * For some obscure reason I couldn't make the texture coordinate
 * translation work correctly with the normal SetTransform(), so I have
 * to transform the texcoords manually.
 */
void TransformTexCoord(float st[2])
{
    D3DXMATRIX *mat = matStack[MAT_TEXTURE]->GetTop();

    // If this is an identity matrix, we don't have to do anything.
    if(*mat == identityMatrix)
        return;

    D3DXVECTOR3 vec(st[0], st[1], 0);
    D3DXVECTOR4 result;
    D3DXVec3Transform(&result, &vec, mat);
    st[0] = result.x;
    st[1] = result.y;
}

void DG_MatrixMode(int mode)
{
    msIndex = (mode == DGL_MODELVIEW? MAT_MODELVIEW
        : mode == DGL_PROJECTION? MAT_PROJECTION
        : mode == DGL_TEXTURE? MAT_TEXTURE
        : msIndex);
}

void DG_PushMatrix(void)
{
    matStack[msIndex]->Push();
}

void DG_PopMatrix(void)
{
    matStack[msIndex]->Pop();
    UploadMatrix();
}

void DG_LoadIdentity(void)
{
    matStack[msIndex]->LoadIdentity();
    UploadMatrix();
}

void DG_Translatef(float x, float y, float z)
{
    matStack[msIndex]->TranslateLocal(x, y, z);
    UploadMatrix();
}

void DG_Rotatef(float angle, float x, float y, float z)
{
    D3DXVECTOR3 axis(x, y, z);
    matStack[msIndex]->RotateAxisLocal(&axis, D3DXToRadian(angle));
    UploadMatrix();
}

void DG_Scalef(float x, float y, float z)
{
    matStack[msIndex]->ScaleLocal(x, y, z);
    UploadMatrix();
}

void DG_Ortho(float left, float top, float right, float bottom,
              float znear, float zfar)
{
    D3DXMATRIX orthoMatrix;
    matStack[msIndex]->MultMatrixLocal(D3DXMatrixOrthoOffCenterLH(
        &orthoMatrix, left, right, bottom, top, znear, zfar));
    UploadMatrix();
}

void DG_Perspective(float fovY, float aspect, float zNear, float zFar)
{
    D3DXMATRIX perMatrix;
    matStack[msIndex]->MultMatrixLocal(D3DXMatrixPerspectiveFovRH(
        &perMatrix, D3DXToRadian(fovY), aspect, zNear, zFar));
    UploadMatrix();
}
