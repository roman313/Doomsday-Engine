/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2007 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2007 Daniel Swanson <danij@dengine.net>
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
 * r_data.h: Data Structures For Refresh
 */

#ifndef __DOOMSDAY_REFRESH_DATA_H__
#define __DOOMSDAY_REFRESH_DATA_H__

#include "gl_main.h"
#include "dd_def.h"
#include "p_think.h"
#include "m_nodepile.h"
#include "def_data.h"
#include "r_extres.h"
#include "s_environ.h"

// Flags for decorations.
#define DCRF_NO_IWAD    0x1         // Don't use if from IWAD.
#define DCRF_PWAD       0x2         // Can use if from PWAD.
#define DCRF_EXTERNAL   0x4         // Can use if from external resource.

// Texture flags.
#define TXF_MASKED      0x1
#define TXF_GLOW        0x2         // For lava etc, textures that glow.
#define TXF_PTCGEN      0x8         // Ptcgen def has been determined.

// Animation group flags.
#define AGF_SMOOTH      0x1
#define AGF_FIRST_ONLY  0x2
#define AGF_TEXTURE     0x1000
#define AGF_FLAT        0x2000
#define AGF_PRECACHE    0x4000      // Group is just for precaching.

// Texture definition
typedef struct {
    short           originx;
    short           originy;
    short           patch;
    short           stepdir;
    short           colormap;
} mappatch_t;

typedef struct {
    char            name[8];
    boolean         masked;
    short           width;
    short           height;
    //void          **columndirectory;  // OBSOLETE
    int             columndirectorypadding;
    short           patchcount;
    mappatch_t      patches[1];
} maptexture_t;

// strifeformat texture definition variants
typedef struct
{
    short   originx;
    short   originy;
    short   patch;
} strifemappatch_t;

typedef struct
{
    char        name[8];
    boolean     unused;
    short       width;
    short       height;
    short       patchcount;
    strifemappatch_t    patches[1];
} strifemaptexture_t;

// Detail texture information.
typedef struct detailinfo_s {
    DGLuint         tex;
    int             width, height;
    float           strength;
    float           scale;
    float           maxdist;
} detailinfo_t;

typedef struct gltexture_s {
    DGLuint         id;
    float           width, height;
    boolean         masked;
    detailinfo_t   *detail;
} gltexture_t;

typedef struct glcommand_vertex_s {
    float           s, t;
    int             index;
} glcommand_vertex_t;

#define RL_MAX_DIVS         64

// Rendpoly flags.
#define RPF_MASKED      0x0001     // Use the special list for masked textures.
#define RPF_SKY_MASK    0x0004     // A sky mask polygon.
#define RPF_LIGHT       0x0008     // A dynamic light.
#define RPF_DYNLIT      0x0010     // Normal list: poly is dynamically lit.
#define RPF_GLOW        0x0020     // Multiply original vtx colors.
#define RPF_DETAIL      0x0040     // Render with detail (incl. vtx distances)
#define RPF_SHADOW      0x0100
#define RPF_HORIZONTAL  0x0200
#define RPF_SHINY       0x0400     // Shiny surface.
#define RPF_DONE        0x8000     // This poly has already been drawn.

typedef enum {
    RP_NONE,
    RP_QUAD,                       // Wall segment.
    RP_DIVQUAD,                    // Divided wall segment.
    RP_FLAT                        // Floor or ceiling.
} rendpolytype_t;

typedef struct {
    float           pos[3];        // X, Y and Z coordinates.
    gl_rgba_t       color;         // Color of the vertex.
} rendpoly_vertex_t;

typedef struct walldiv_s {
    unsigned int    num;
    float           pos[RL_MAX_DIVS];
} walldiv_t;

typedef struct rendpoly_wall_s {
    float           length;
    walldiv_t       divs[2];       // For wall segments (two vertices).
} rendpoly_wall_t;

// rendpoly_t is only for convenience; the data written in the rendering
// list data buffer is taken from this struct.
typedef struct rendpoly_s {
    boolean         isWall;
    rendpolytype_t  type;
    short           flags;         // RPF_*.
    float           texoffx, texoffy;   /* Texture coordinates for left/top
                                           (in real texcoords). */
    gltexture_t     tex;
    gltexture_t     intertex;
    float           interpos;      // Blending strength (0..1).
    uint            lightListIdx;  // List of lights that affect this poly.
    DGLuint         decorlightmap; // Pregen RGB lightmap for decor lights.
    blendmode_t     blendmode;     // Primitive-specific blending mode.

    // The geometry:
    float           normal[3];
    byte            numvertices;   // Number of vertices for the poly.
    rendpoly_vertex_t *vertices;

    rendpoly_wall_t *wall;         // Wall specific data if any.
} rendpoly_t;

// This is the dummy mobj_t used for blockring roots.
// It has some excess information since it has to be compatible with
// regular mobjs (otherwise the rings don't really work).
// Note: the thinker and pos data could be used for something else...
typedef struct linkmobj_s {
    thinker_t       thinker;
    fixed_t         pos[3];
    struct mobj_s  *next, *prev;
} linkmobj_t;

// Shadowpoly flags.
#define SHPF_FRONTSIDE  0x1

typedef struct shadowpolyoffset_s {
    float           offset[2];
} shadowpolyoffset_t;

#define MAX_EXOFFSETS   8
#define MAX_BEXOFFSETS  8

typedef struct shadowpoly_s {
    struct seg_s   *seg;
    struct subsector_s *ssec;
    short           flags;
    ushort          visframe;      // Last visible frame (for rendering).
    struct vertex_s *outer[2];      // Left and right.
    float           inoffset[2][2]; // Inner offset from 'outer.'
    float           extoffset[2][2];    // Extended: offset from 'outer.'
    shadowpolyoffset_t bextoffset[2][MAX_BEXOFFSETS];   // Back-extended: offset frmo 'outer.'
} shadowpoly_t;

typedef struct shadowlink_s {
    struct shadowlink_s *next;
    shadowpoly_t   *poly;
} shadowlink_t;

typedef struct subplaneinfo_s {
    int             type;           // Plane type (ie PLN_FLOOR or PLN_CEILING)
    vertexillum_t  *illumination;
    biastracker_t   tracker;
    uint            updated;
    biasaffection_t affected[MAX_BIAS_AFFECTED];
} subplaneinfo_t;

typedef struct {
    float           rgb[3];
} rgbcol_t;

typedef struct {
    int             originx;       // block origin (allways UL), which has allready
    int             originy;       // accounted  for the patch's internal origin
    int             patch;
} texpatch_t;

// Flags for texinfo.
#define TXIF_MONOCHROME             0x1
#define TXIF_UPSCALE_AND_SHARPEN    0x2

typedef struct {
    short           width;
    short           height;
    short           offsetX;
    short           offsetY;
    int             modFlags;      // Possible modifier filters to apply (monochrome, scale+sharp)
    byte            masked;        // Is the (DGL) texture masked?
    detailinfo_t    detail;        // Detail texture information.
} texinfo_t;

// a maptexturedef_t describes a rectangular texture, which is composed of one
// or more mappatch_t structures that arrange graphic patches
typedef struct {
    DGLuint         tex;           // Name of the associated DGL texture.
    char            name[9];       // for switch changing, etc; ends in \0
    texinfo_t       info;
    int             flags;         // TXF_* flags.
    rgbcol_t        color;
    byte            ingroup;       // True if texture belongs to some animgroup.
    materialtype_t  materialType;  // Used for environmental sound properties.
    struct ded_decor_s *decoration; /* Pointer to the surface
                                     * decoration, if any. */
    struct ded_reflection_s *reflection; // Surface reflection definition.

    short           patchcount;
    texpatch_t      patches[1];    // [patchcount] drawn back to front
} texture_t;                       //   into the cached texture

typedef struct {
    DGLuint         tex;
    texinfo_t       info;
} ddtexture_t;

typedef struct translation_s {
    int             current;
    int             next;
    float           inter;
} translation_t;

typedef struct flat_s {
    DGLuint         tex;            // Name of the associated DGL texture.
    char            name[9];        // for switch changing, etc; ends in \0
    texinfo_t       info;
    short           flags;
    rgbcol_t        color;
    byte            ingroup;        // True if belongs to some animgroup.
    materialtype_t  materialType;   // Used for environmental sound properties.
    struct ded_decor_s *decoration; // Pointer to the surface decoration,
                                    // if any.
    struct ded_reflection_s *reflection; // Surface reflection definition.

    int             lump;
    struct ded_ptcgen_s *ptcgen;    // Particle generator for the flat.
} flat_t;

// a patch is a lumppatch that has been prepared for render.
typedef struct patch_s {
    int             lump;
    short           offx, offy;

    // Part 1
    DGLuint         tex;          // Name of the associated DGL texture.
    texinfo_t       info;

    // Part 2 (only used with textures larger than the max texture size).
    DGLuint         tex2;
    texinfo_t       info2;

    struct patch_s *next;
} patch_t;

// A rawtex is a lump raw graphic that has been prepared for render.
typedef struct rawtex_s {
    int             lump;
    // Part 1
    DGLuint         tex;          // Name of the associated DGL texture.
    texinfo_t       info;

    // Part 2 (only used with textures larger than the max texture size).
    DGLuint         tex2;
    texinfo_t       info2;
} rawtex_t;

typedef struct animframe_s {
    int             number;
    ushort          tics;
    ushort          random;
} animframe_t;

typedef struct animgroup_s {
    int             id;
    int             flags;
    int             index;
    int             maxtimer;
    int             timer;
    int             count;
    animframe_t    *frames;
} animgroup_t;

typedef struct {
    boolean         used;
    float           approxDist;         // Only an approximation.
    struct lumobj_s *lum;
    float           worldVector[3];     // Light direction vector (world space).
    float           vector[3];          // Light direction vector (object space).
    float           color[3];           // How intense the light is (0..1, RGB).
    float           offset;
    float           lightSide, darkSide;    // Factors for world light.
} vlight_t;

extern nodeindex_t *linelinks;
extern long    *blockmaplump;      // offsets in blockmap are from here
extern long    *blockmap;
extern int      bmapwidth, bmapheight;  // in mapblocks
extern fixed_t  bmaporgx, bmaporgy; // origin of block map
extern linkmobj_t *blockrings;
extern polyblock_t **polyblockmap;
extern byte    *rejectmatrix;      // for fast sight rejection
extern nodepile_t *thingnodes, *linenodes;

extern int      viewwidth, viewheight;
extern int      numtextures;
extern texture_t **textures;
extern translation_t *texturetranslation;   // for global animation
extern int      numflats;
extern flat_t **flats;
extern translation_t *flattranslation;   // for global animation
extern rawtex_t *rawtextures;
extern uint numrawtextures;
extern int      numgroups;
extern animgroup_t *groups;
extern int      levelFullBright;
extern int      glowingTextures;
extern byte     precacheSprites, precacheSkins;

void            R_InitRendPolyPool(void);
rendpoly_t     *R_AllocRendPoly(rendpolytype_t type, boolean isWall,
                                unsigned int numverts);
void            R_FreeRendPoly(rendpoly_t *poly);
void            R_MemcpyRendPoly(rendpoly_t *dest, const rendpoly_t *src);
void            R_InfoRendPolys(void);

void            R_InitData(void);
void            R_UpdateData(void);
void            R_ShutdownData(void);
void            R_UpdateSector(struct sector_s *sec, boolean forceUpdate);
void            R_UpdateSurface(surface_t *current, boolean forceUpdate);
void            R_UpdateAllSurfaces(boolean forceUpdate);
void            R_PrecacheLevel(void);
void            R_PrecachePatch(int lumpnum);
void            R_InitAnimGroup(ded_group_t *def);
void            R_ResetAnimGroups(void);
boolean         R_IsInAnimGroup(int groupNum, int type, int number);
void            R_AnimateAnimGroups(void);
int             R_GraphicResourceFlags(resourceclass_t rclass, int id);
patch_t        *R_FindPatch(int lumpnum);    // May return NULL.
patch_t        *R_GetPatch(int lumpnum); // Creates new entries.
patch_t       **R_CollectPatches(int *count);
rawtex_t       *R_FindRawTex(uint lumpnum);    // May return NULL.
rawtex_t       *R_GetRawTex(uint lumpnum); // Creates new entries.
int             R_CheckFlatNumForName(const char *name);
int             R_FlatNumForName(const char *name);
const char     *R_FlatNameForNum(int num);
int             R_CheckTextureNumForName(const char *name);
int             R_TextureNumForName(const char *name);
const char     *R_TextureNameForNum(int num);
int             R_SetFlatTranslation(int flat, int translateTo);
int             R_SetTextureTranslation(int tex, int translateTo);
boolean         R_IsCustomTexture(int texture);
boolean         R_IsAllowedDecoration(ded_decor_t *def, int index,
                                      boolean hasExternal);
boolean         R_IsValidLightDecoration(ded_decorlight_t *lightDef);
void            R_GenerateDecorMap(ded_decor_t *def);

#endif
