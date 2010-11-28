/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2010 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2010 Daniel Swanson <danij@dengine.net>
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

/**
 * InFine: "Finale" script interpreter.
 */

// HEADER FILES ------------------------------------------------------------

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "de_base.h"
#include "de_console.h"
#include "de_play.h"
#include "de_defs.h"
#include "de_graphics.h"
#include "de_refresh.h"
#include "de_render.h"
#include "de_network.h"
#include "de_audio.h"
#include "de_infine.h"
#include "de_misc.h"
#include "de_infine.h"
#include "de_ui.h"

// MACROS ------------------------------------------------------------------

#define FRACSECS_TO_TICKS(sec) ((int)(sec * TICSPERSEC + 0.5))

// Helper macro for defining infine command functions.
#define DEFFC(name) void FIC_##name(const struct command_s* cmd, const fi_operand_t* ops, finaleinterpreter_t* fi)

// Helper macro for accessing the value of an operand.
#define OP_INT(n)           (ops[n].data.integer)
#define OP_FLOAT(n)         (ops[n].data.flt)
#define OP_CSTRING(n)       (ops[n].data.cstring)
#define OP_OBJECT(n)        (ops[n].data.obj)

// TYPES -------------------------------------------------------------------

typedef enum {
    FVT_INT,
    FVT_FLOAT,
    FVT_SCRIPT_STRING, // ptr points to a char*, which points to the string.
    FVT_OBJECT
} fi_operand_type_t;

typedef struct {
    fi_operand_type_t type;
    union {
        int         integer;
        float       flt;
        const char* cstring;
        fi_object_t* obj;
    } data;
} fi_operand_t;

typedef struct command_s {
    char*           token;
    const char*     operands;
    void          (*func) (const struct command_s* cmd, const fi_operand_t* ops, finaleinterpreter_t* fi);
    struct command_flags_s {
        char            when_skipping:1;
        char            when_condition_skipping:1; // Skipping because condition failed.
    } flags;
} command_t;

typedef struct fi_namespace_record_s {
    fi_objectname_t name; // Unique among objects of the same type and spawned by the same script.
    fi_objectid_t   objectId;
} fi_namespace_record_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// Command functions.
DEFFC(Do);
DEFFC(End);
DEFFC(BGFlat);
DEFFC(BGTexture);
DEFFC(NoBGMaterial);
DEFFC(Wait);
DEFFC(WaitText);
DEFFC(WaitAnim);
DEFFC(Tic);
DEFFC(InTime);
DEFFC(Color);
DEFFC(ColorAlpha);
DEFFC(Pause);
DEFFC(CanSkip);
DEFFC(NoSkip);
DEFFC(SkipHere);
DEFFC(Events);
DEFFC(NoEvents);
DEFFC(OnKey);
DEFFC(UnsetKey);
DEFFC(If);
DEFFC(IfNot);
DEFFC(Else);
DEFFC(GoTo);
DEFFC(Marker);
DEFFC(Image);
DEFFC(ImageAt);
DEFFC(XImage);
DEFFC(Delete);
DEFFC(Patch);
DEFFC(SetPatch);
DEFFC(Anim);
DEFFC(AnimImage);
DEFFC(StateAnim);
DEFFC(Repeat);
DEFFC(ClearAnim);
DEFFC(PicSound);
DEFFC(ObjectOffX);
DEFFC(ObjectOffY);
DEFFC(ObjectOffZ);
DEFFC(ObjectScaleX);
DEFFC(ObjectScaleY);
DEFFC(ObjectScaleZ);
DEFFC(ObjectScale);
DEFFC(ObjectScaleXY);
DEFFC(ObjectScaleXYZ);
DEFFC(ObjectRGB);
DEFFC(ObjectAlpha);
DEFFC(ObjectAngle);
DEFFC(Rect);
DEFFC(FillColor);
DEFFC(EdgeColor);
DEFFC(OffsetX);
DEFFC(OffsetY);
DEFFC(Sound);
DEFFC(SoundAt);
DEFFC(SeeSound);
DEFFC(DieSound);
DEFFC(Music);
DEFFC(MusicOnce);
DEFFC(Filter);
DEFFC(Text);
DEFFC(TextFromDef);
DEFFC(TextFromLump);
DEFFC(SetText);
DEFFC(SetTextDef);
DEFFC(DeleteText);
DEFFC(Font);
DEFFC(FontA);
DEFFC(FontB);
DEFFC(PredefinedTextColor);
DEFFC(TextRGB);
DEFFC(TextAlpha);
DEFFC(TextOffX);
DEFFC(TextOffY);
DEFFC(TextCenter);
DEFFC(TextNoCenter);
DEFFC(TextScroll);
DEFFC(TextPos);
DEFFC(TextRate);
DEFFC(TextLineHeight);
DEFFC(NoMusic);
DEFFC(TextScaleX);
DEFFC(TextScaleY);
DEFFC(TextScale);
DEFFC(PlayDemo);
DEFFC(Command);
DEFFC(ShowMenu);
DEFFC(NoShowMenu);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static fi_objectid_t toObjectId(fi_namespace_t* names, const char* name, fi_obtype_e type);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

/// Time is measured in seconds.
/// Colors are floating point and [0,1].
static const command_t commands[] = {
    // Run Control
    { "DO",         "", FIC_Do, true, true },
    { "END",        "", FIC_End },
    { "IF",         "s", FIC_If }, // if (value-id)
    { "IFNOT",      "s", FIC_IfNot }, // ifnot (value-id)
    { "ELSE",       "", FIC_Else },
    { "GOTO",       "s", FIC_GoTo }, // goto (marker)
    { "MARKER",     "s", FIC_Marker, true },
    { "in",         "f", FIC_InTime }, // in (time)
    { "pause",      "", FIC_Pause },
    { "tic",        "", FIC_Tic },
    { "wait",       "f", FIC_Wait }, // wait (time)
    { "waittext",   "s", FIC_WaitText }, // waittext (id)
    { "waitanim",   "s", FIC_WaitAnim }, // waitanim (id)
    { "canskip",    "", FIC_CanSkip },
    { "noskip",     "", FIC_NoSkip },
    { "skiphere",   "", FIC_SkipHere, true },
    { "events",     "", FIC_Events },
    { "noevents",   "", FIC_NoEvents },
    { "onkey",      "ss", FIC_OnKey }, // onkey (keyname) (marker)
    { "unsetkey",   "s", FIC_UnsetKey }, // unsetkey (keyname)

    // Screen Control
    { "color",      "fff", FIC_Color }, // color (red) (green) (blue)
    { "coloralpha", "ffff", FIC_ColorAlpha }, // coloralpha (r) (g) (b) (a)
    { "flat",       "s", FIC_BGFlat }, // flat (flat-id)
    { "texture",    "s", FIC_BGTexture }, // texture (texture-id)
    { "noflat",     "", FIC_NoBGMaterial },
    { "notexture",  "", FIC_NoBGMaterial },
    { "offx",       "f", FIC_OffsetX }, // offx (x)
    { "offy",       "f", FIC_OffsetY }, // offy (y)
    { "filter",     "ffff", FIC_Filter }, // filter (r) (g) (b) (a)

    // Audio
    { "sound",      "s", FIC_Sound }, // sound (snd)
    { "soundat",    "sf", FIC_SoundAt }, // soundat (snd) (vol:0-1)
    { "seesound",   "s", FIC_SeeSound }, // seesound (mobjtype)
    { "diesound",   "s", FIC_DieSound }, // diesound (mobjtype)
    { "music",      "s", FIC_Music }, // music (musicname)
    { "musiconce",  "s", FIC_MusicOnce }, // musiconce (musicname)
    { "nomusic",    "", FIC_NoMusic },

    // Objects
    { "del",        "o", FIC_Delete }, // del (obj)
    { "x",          "of", FIC_ObjectOffX }, // x (obj) (x)
    { "y",          "of", FIC_ObjectOffY }, // y (obj) (y)
    { "z",          "of", FIC_ObjectOffZ }, // z (obj) (z)
    { "sx",         "of", FIC_ObjectScaleX }, // sx (obj) (x)
    { "sy",         "of", FIC_ObjectScaleY }, // sy (obj) (y)
    { "sz",         "of", FIC_ObjectScaleZ }, // sz (obj) (z)
    { "scale",      "of", FIC_ObjectScale }, // scale (obj) (factor)
    { "scalexy",    "off", FIC_ObjectScaleXY }, // scalexy (obj) (x) (y)
    { "scalexyz",   "offf", FIC_ObjectScaleXYZ }, // scalexyz (obj) (x) (y) (z)
    { "rgb",        "offf", FIC_ObjectRGB }, // rgb (obj) (r) (g) (b)
    { "alpha",      "of", FIC_ObjectAlpha }, // alpha (obj) (alpha)
    { "angle",      "of", FIC_ObjectAngle }, // angle (obj) (degrees)

    // Rects
    { "rect",       "sffff", FIC_Rect }, // rect (hndl) (x) (y) (w) (h)
    { "fillcolor",  "osffff", FIC_FillColor }, // fillcolor (obj) (top/bottom/both) (r) (g) (b) (a)
    { "edgecolor",  "osffff", FIC_EdgeColor }, // edgecolor (obj) (top/bottom/both) (r) (g) (b) (a)

    // Pics
    { "image",      "ss", FIC_Image }, // image (id) (raw-image-lump)
    { "imageat",    "sffs", FIC_ImageAt }, // imageat (id) (x) (y) (raw)
    { "ximage",     "ss", FIC_XImage }, // ximage (id) (ext-gfx-filename)
    { "patch",      "sffs", FIC_Patch }, // patch (id) (x) (y) (patch)
    { "set",        "ss", FIC_SetPatch }, // set (id) (lump)
    { "clranim",    "o", FIC_ClearAnim }, // clranim (obj)
    { "anim",       "ssf", FIC_Anim }, // anim (id) (patch) (time)
    { "imageanim",  "ssf", FIC_AnimImage }, // imageanim (id) (raw-img) (time)
    { "picsound",   "ss", FIC_PicSound }, // picsound (id) (sound)
    { "repeat",     "s", FIC_Repeat }, // repeat (id)
    { "states",     "ssi", FIC_StateAnim }, // states (id) (state) (count)

    // Text
    { "text",       "sffs", FIC_Text }, // text (hndl) (x) (y) (string)
    { "textdef",    "sffs", FIC_TextFromDef }, // textdef (hndl) (x) (y) (txt-id)
    { "textlump",   "sffs", FIC_TextFromLump }, // textlump (hndl) (x) (y) (lump)
    { "settext",    "ss", FIC_SetText }, // settext (id) (newtext)
    { "settextdef", "ss", FIC_SetTextDef }, // settextdef (id) (txt-id)
    { "center",     "s", FIC_TextCenter }, // center (id)
    { "nocenter",   "s", FIC_TextNoCenter }, // nocenter (id)
    { "scroll",     "sf", FIC_TextScroll }, // scroll (id) (speed)
    { "pos",        "si", FIC_TextPos }, // pos (id) (pos)
    { "rate",       "si", FIC_TextRate }, // rate (id) (rate)
    { "font",       "ss", FIC_Font }, // font (id) (font)
    { "fonta",      "s", FIC_FontA }, // fonta (id)
    { "fontb",      "s", FIC_FontB }, // fontb (id)
    { "linehgt",    "sf", FIC_TextLineHeight }, // linehgt (hndl) (hgt)

    // Game Control
    { "playdemo",   "s", FIC_PlayDemo }, // playdemo (filename)
    { "cmd",        "s", FIC_Command }, // cmd (console command)
    { "trigger",    "", FIC_ShowMenu },
    { "notrigger",  "", FIC_NoShowMenu },

    // Misc.
    { "precolor",   "ifff", FIC_PredefinedTextColor }, // precolor (num) (r) (g) (b)

    // Deprecated Pic commands
    { "delpic",     "o", FIC_Delete }, // delpic (obj)

    // Deprecated Text commands
    { "deltext",    "o", FIC_DeleteText }, // deltext (obj)
    { "textrgb",    "sfff", FIC_TextRGB }, // textrgb (id) (r) (g) (b)
    { "textalpha",  "sf", FIC_TextAlpha }, // textalpha (id) (alpha)
    { "tx",         "sf", FIC_TextOffX }, // tx (id) (x)
    { "ty",         "sf", FIC_TextOffY }, // ty (id) (y)
    { "tsx",        "sf", FIC_TextScaleX }, // tsx (id) (x)
    { "tsy",        "sf", FIC_TextScaleY }, // tsy (id) (y)
    { "textscale",  "sf", FIC_TextScale }, // textscale (id) (x) (y)

    { NULL, 0, NULL } // Terminate.
};

// CODE --------------------------------------------------------------------

static fi_objectid_t findIdForName(fi_namespace_t* names, const char* name)
{
    fi_objectid_t id;
    // First check all pics.
    id = toObjectId(names, name, FI_PIC);
    // Then check text objects.
    if(!id)
        id = toObjectId(names, name, FI_TEXT);
    return id;
}

static fi_objectid_t toObjectId(fi_namespace_t* names, const char* name, fi_obtype_e type)
{
    assert(name && name[0]);
    if(type == FI_NONE)
    {   // Use a priority-based search.
        return findIdForName(names, name);
    }

    {uint i;
    for(i = 0; i < names->num; ++i)
    {
        fi_namespace_record_t* rec = &names->vector[i];
        if(!stricmp(rec->name, name) && FI_Object(rec->objectId)->type == type)
            return rec->objectId;
    }}
    return 0;
}

static void destroyObjectsInScope(fi_namespace_t* names)
{
    // Delete external images, text strings etc.
    if(names->vector)
    {
        uint i;
        for(i = 0; i < names->num; ++i)
        {
            fi_namespace_record_t* rec = &names->vector[i];
            FI_DeleteObject(FI_Object(rec->objectId));
        }
        Z_Free(names->vector);
    }
    names->vector = NULL;
    names->num = 0;
}

static uint objectIndexInNamespace(fi_namespace_t* names, fi_object_t* obj)
{
    if(obj)
    {
        uint i;
        for(i = 0; i < names->num; ++i)
        {
            fi_namespace_record_t* rec = &names->vector[i];
            if(rec->objectId == obj->id)
                return i+1;
        }
    }
    return 0;
}

static __inline boolean objectInNamespace(fi_namespace_t* names, fi_object_t* obj)
{
    return objectIndexInNamespace(names, obj) != 0;
}

/**
 * \note Does not check if the object already exists in this scope.
 */
static fi_object_t* addObjectToNamespace(fi_namespace_t* names, const char* name, fi_object_t* obj)
{
    fi_namespace_record_t* rec;
    names->vector = Z_Realloc(names->vector, sizeof(*names->vector) * ++names->num, PU_STATIC);
    rec = &names->vector[names->num-1];
    
    rec->objectId = obj->id;
    memset(rec->name, 0, sizeof(rec->name));
    dd_snprintf(rec->name, FI_NAME_MAX_LENGTH, "%s", name);

    return obj;
}

/**
 * \assume There is at most one reference to the object in this scope.
 */
static fi_object_t* removeObjectInNamespace(fi_namespace_t* names, fi_object_t* obj)
{
    uint idx;
    if((idx = objectIndexInNamespace(names, obj)))
    {
        idx -= 1; // Indices are 1-based.

        if(idx != names->num-1)
            memmove(&names->vector[idx], &names->vector[idx+1], sizeof(*names->vector) * (names->num-idx));

        if(names->num > 1)
        {
            names->vector = Z_Realloc(names->vector, sizeof(*names->vector) * --names->num, PU_STATIC);
        }
        else
        {
            Z_Free(names->vector);
            names->vector = NULL;
            names->num = 0;
        }
    }
    return obj;
}

static fi_objectid_t findObjectIdForName(fi_namespace_t* names, const char* name, fi_obtype_e type)
{
    if(!name || !name[0])
        return 0;
    return toObjectId(names, name, type);
}

static const command_t* findCommand(const char* name)
{
    size_t i;
    for(i = 0; commands[i].token; ++i)
    {
        const command_t* cmd = &commands[i];
        if(!stricmp(cmd->token, name))
            return cmd;
    }
    return 0; // Not found.
}

static void releaseScript(finaleinterpreter_t* fi)
{
    if(fi->_script)
        Z_Free(fi->_script);
    fi->_script = 0;
    fi->_cp = 0;
}

static const char* nextToken(finaleinterpreter_t* fi)
{
    char* out;

    // Skip whitespace.
    while(*fi->_cp && isspace(*fi->_cp))
        fi->_cp++;
    if(!*fi->_cp)
        return NULL; // The end has been reached.

    out = fi->_token;
    if(*fi->_cp == '"') // A string?
    {
        for(fi->_cp++; *fi->_cp; fi->_cp++)
        {
            if(*fi->_cp == '"')
            {
                fi->_cp++;
                // Convert double quotes to single ones.
                if(*fi->_cp != '"')
                    break;
            }
            *out++ = *fi->_cp;
        }
    }
    else
    {
        while(!isspace(*fi->_cp) && *fi->_cp)
            *out++ = *fi->_cp++;
    }
    *out++ = 0;

    return fi->_token;
}

/**
 * Parse the command operands from the script. If successfull, a ptr to a new
 * vector of @c fi_operand_t objects is returned. Ownership of the vector is
 * given to the caller.
 *
 * @return  Ptr to a new vector of @c fi_operand_t or @c NULL.
 */
static fi_operand_t* parseCommandArguments(finaleinterpreter_t* fi, const command_t* cmd, uint* count)
{
    const char* origCursorPos;
    uint numOperands;
    fi_operand_t* ops = 0;

    if(!cmd->operands || !cmd->operands[0])
        return NULL;

    origCursorPos = fi->_cp;
    numOperands = (uint)strlen(cmd->operands);

    // Operands are read sequentially.
    {uint i = 0;
    do
    {
        char typeSymbol = cmd->operands[i];
        fi_operand_t* op;
        fi_operand_type_t type;

        if(!nextToken(fi))
        {
            fi->_cp = origCursorPos;
            if(ops)
                free(ops);
            if(count)
                *count = 0;
            Con_Message("parseCommandArguments: Too few operands for command '%s'.\n", cmd->token);
            return NULL;
        }

        switch(typeSymbol)
        {
        // Supported operand type symbols:
        case 'i': type = FVT_INT;           break;
        case 'f': type = FVT_FLOAT;         break;
        case 's': type = FVT_SCRIPT_STRING; break;
        case 'o': type = FVT_OBJECT;        break;
        default:
            Con_Error("parseCommandArguments: Invalid symbol '%c' in operand list for command '%s'.", typeSymbol, cmd->token);
        }

        if(!ops)
            ops = malloc(sizeof(*ops));
        else
            ops = realloc(ops, sizeof(*ops) * (i+1));
        op = &ops[i];

        op->type = type;
        switch(type)
        {
        case FVT_INT:
            op->data.integer = strtol(fi->_token, NULL, 0);
            break;

        case FVT_FLOAT:
            op->data.flt = strtod(fi->_token, NULL);
            break;
        case FVT_SCRIPT_STRING:
            {
            size_t len = strlen(fi->_token)+1;
            char* str = malloc(len);
            dd_snprintf(str, len, "%s", fi->_token);
            op->data.cstring = str;
            break;
            }
        case FVT_OBJECT:
            op->data.obj = FI_Object(findObjectIdForName(&fi->_namespace, fi->_token, FI_NONE));
            break;
        }
    } while(++i < numOperands);}

    fi->_cp = origCursorPos;

    if(count)
        *count = numOperands;
    return ops;
}

static boolean skippingCommand(finaleinterpreter_t* fi, const command_t* cmd)
{
    if((fi->_skipNext && !cmd->flags.when_condition_skipping) ||
       ((fi->_skipping || fi->_gotoSkip) && !cmd->flags.when_skipping))
    {
        // While not DO-skipping, the condskip has now been done.
        if(!fi->_doLevel)
        {
            if(fi->_skipNext)
                fi->_lastSkipped = true;
            fi->_skipNext = false;
        }
        return true;
    }
    return false;
}

/**
 * Execute one (the next) command, advance script cursor.
 */
static boolean executeCommand(finaleinterpreter_t* fi, const char* commandString)
{
    // Semicolon terminates DO-blocks.
    if(!strcmp(commandString, ";"))
    {
        if(fi->_doLevel > 0)
        {
            if(--fi->_doLevel == 0)
            {
                // The DO-skip has been completed.
                fi->_skipNext = false;
                fi->_lastSkipped = true;
            }
        }
        return true; // Success!
    }

    // We're now going to execute a command.
    fi->_cmdExecuted = true;
    // So unhide our UI page(s).
    FIPage_MakeVisible(fi->_pages[PAGE_PICS], true);
    FIPage_MakeVisible(fi->_pages[PAGE_TEXT], true);

    // Is this a command we know how to execute?
    {const command_t* cmd;
    if((cmd = findCommand(commandString)))
    {
        boolean requiredOperands = (cmd->operands && cmd->operands[0]);
        fi_operand_t* ops = NULL;
        uint numOps;

        // Check that there are enough operands.
        if(!requiredOperands || (ops = parseCommandArguments(fi, cmd, &numOps)))
        {
            // Should we skip this command?
            if(skippingCommand(fi, cmd))
                return false;

            // Execute forthwith!
            cmd->func(cmd, ops, fi);
        }

        if(fi->_gotoEnd)
        {
            fi->_wait = 1;
        }
        else
        {   // Now we've executed the latest command.
            fi->_lastSkipped = false;
        }

        if(ops)
        {
            uint i;
            for(i = 0; i < numOps; ++i)
            {
                fi_operand_t* op = &ops[i];
                if(op->type == FVT_SCRIPT_STRING)
                    free((char*)op->data.cstring);
            }
            free(ops);
        }
    }}
    return true; // Success!
}

static boolean executeNextCommand(finaleinterpreter_t* fi)
{
    const char* token;
    if((token = nextToken(fi)))
    {
        executeCommand(fi, token);
        return true;
    }
    return false;
}

static __inline uint pageForObjectType(fi_obtype_e type)
{
    return (type == FI_TEXT? PAGE_TEXT : PAGE_PICS);
}

/**
 * Find an @c fi_object_t of type with the type-unique name.
 * @param name  Unique name of the object we are looking for.
 * @return  Ptr to @c fi_object_t Either:
 *          a) Existing object associated with unique @a name.
 *          b) New object with unique @a name.
 */
static fi_object_t* getObject(finaleinterpreter_t* fi, fi_obtype_e type, const char* name)
{
    assert(name && name);
    {
    fi_objectid_t id;
    // An existing object?
    if((id = findObjectIdForName(&fi->_namespace, name, type)))
        return FI_Object(id);
    return FIPage_AddObject(fi->_pages[pageForObjectType(type)], addObjectToNamespace(&fi->_namespace, name, FI_NewObject(type, name)));
    }
}

static void clearEventHandlers(finaleinterpreter_t* fi)
{
    if(fi->_numEventHandlers)
        Z_Free(fi->_eventHandlers);
    fi->_eventHandlers = 0;
    fi->_numEventHandlers = 0;
}

static fi_handler_t* findEventHandler(finaleinterpreter_t* fi, const ddevent_t* ev)
{
    uint i;
    for(i = 0; i < fi->_numEventHandlers; ++i)
    {
        fi_handler_t* h = &fi->_eventHandlers[i];
        if(h->ev.device != ev->device && h->ev.type != ev->type)
            continue;
        switch(h->ev.type)
        {
        case E_TOGGLE:
            if(h->ev.toggle.id != ev->toggle.id)
                continue;
            break;
        case E_AXIS:
            if(h->ev.axis.id != ev->axis.id)
                continue;
            break;
        case E_ANGLE:
            if(h->ev.angle.id != ev->angle.id)
                continue;
            break;
        default:
            Con_Error("Internal error: Invalid event template (type=%i) in finale event handler.", (int) h->ev.type);
        }
        return h;
    }
    return 0;
}

static fi_handler_t* createEventHandler(finaleinterpreter_t* fi, const ddevent_t* ev, const char* marker)
{
    fi_handler_t* h;
    fi->_eventHandlers = Z_Realloc(fi->_eventHandlers, sizeof(*h) * ++fi->_numEventHandlers, PU_STATIC);
    h = &fi->_eventHandlers[fi->_numEventHandlers-1];
    memset(h, 0, sizeof(*h));
    memcpy(&h->ev, ev, sizeof(h->ev));
    dd_snprintf(h->marker, FI_NAME_MAX_LENGTH, "%s", marker);
    return h;
}

static void destroyEventHandler(finaleinterpreter_t* fi, fi_handler_t* h)
{
    assert(fi && h);
    {uint i;
    for(i = 0; i < fi->_numEventHandlers; ++i)
    {
        fi_handler_t* other = &fi->_eventHandlers[i];

        if(h != other)
            continue;

        if(i != fi->_numEventHandlers-1)
            memmove(&fi->_eventHandlers[i], &fi->_eventHandlers[i+1], sizeof(*fi->_eventHandlers) * (fi->_numEventHandlers-i));

        // Resize storage?
        if(fi->_numEventHandlers > 1)
        {
            fi->_eventHandlers = Z_Realloc(fi->_eventHandlers, sizeof(*fi->_eventHandlers) * --fi->_numEventHandlers, PU_STATIC);
        }
        else
        {
            Z_Free(fi->_eventHandlers);
            fi->_eventHandlers = NULL;
            fi->_numEventHandlers = 0;
        }
        return;
    }}
}

/**
 * Find a @c fi_handler_t for the specified ddkey code.
 * @param ev  Input event to find a handler for.
 * @return  Ptr to @c fi_handler_t object. Either:
 *          a) Existing handler associated with unique @a code.
 *          b) New object with unique @a code.
 */
static boolean getEventHandler(finaleinterpreter_t* fi, const ddevent_t* ev, const char* marker)
{
    // First, try to find an existing handler.
    if(findEventHandler(fi, ev))
        return true;
    // Allocate and attach another.
    createEventHandler(fi, ev, marker);
    return true;
}

static void stopScript(finaleinterpreter_t* fi)
{
#ifdef _DEBUG
    Con_Printf("Finale End - id:%i '%.30s'\n", fi->_id, fi->_script);
#endif
    fi->flags.stopped = true;
    if(isServer && !(FI_ScriptFlags(fi->_id) & FF_LOCAL))
    {   // Tell clients to stop the finale.
        Sv_Finale(FINF_END, 0);
    }
    // Any hooks?
    Plug_DoHook(HOOK_FINALE_SCRIPT_STOP, fi->_id, 0);
}

static void changePageBackground(fi_page_t* p, material_t* mat)
{
    // If the page does not yet have a background set we must setup the color+alpha.
    if(mat && !FIPage_BackgroundMaterial(p))
    {
        FIPage_SetBackgroundTopColorAndAlpha(p, 1, 1, 1, 1, 0);
        FIPage_SetBackgroundBottomColorAndAlpha(p, 1, 1, 1, 1, 0);
    }
    FIPage_SetBackgroundMaterial(p, mat);
}

finaleinterpreter_t* P_CreateFinaleInterpreter(void)
{
    return Z_Calloc(sizeof(finaleinterpreter_t), PU_STATIC, 0);
}

void P_DestroyFinaleInterpreter(finaleinterpreter_t* fi)
{
    assert(fi);
    stopScript(fi);
    clearEventHandlers(fi);
    releaseScript(fi);
    destroyObjectsInScope(&fi->_namespace);
    FI_DeletePage(fi->_pages[PAGE_PICS]);
    FI_DeletePage(fi->_pages[PAGE_TEXT]);
    Z_Free(fi);
}

void FinaleInterpreter_LoadScript(finaleinterpreter_t* fi, const char* script)
{
    assert(fi && script && script[0]);
    {
    size_t size = strlen(script);

    /**
     * InFine imposes a strict object drawing order:
     *
     * 1: Background flat (or a single-color background).
     * 2: Picture objects (globally offseted with OffX and OffY), in the order in which they were created.
     * 3: Text objects, in the order in which they were created.
     * 4: Filter.
     *
     * For this we'll need two pages; one for it's background and for Pics and another for Text and it's filter.
     */
    fi->_pages[PAGE_PICS] = FI_NewPage(0);
    fi->_pages[PAGE_TEXT] = FI_NewPage(0);

    // Configure the predefined colors. We want the same for both pages.
    { uint i;
    for(i = 0; i < FIPAGE_NUM_PREDEFINED_COLORS; ++i)
    {
        ui_color_t* color = UI_Color(i);
        FIPage_SetPredefinedColor(fi->_pages[PAGE_PICS], i, color->red, color->green, color->blue, 0);
        FIPage_SetPredefinedColor(fi->_pages[PAGE_TEXT], i, color->red, color->green, color->blue, 0);
    }}

    // Hide our pages until command interpretation begins.
    FIPage_MakeVisible(fi->_pages[PAGE_PICS], false);
    FIPage_MakeVisible(fi->_pages[PAGE_TEXT], false);

    // Take a copy of the script.
    fi->_script = Z_Realloc(fi->_script, size + 1, PU_STATIC);
    memcpy(fi->_script, script, size);
    fi->_script[size] = '\0';
    fi->_cp = fi->_script; // Init cursor.
    fi->flags.suspended = false;
    fi->flags.paused = false;
    fi->flags.show_menu = true; // Enabled by default.
    fi->flags.can_skip = true; // By default skipping is enabled.

    fi->_cmdExecuted = false; // Nothing is drawn until a cmd has been executed.
    fi->_skipping = false;
    fi->_wait = 0; // Not waiting for anything.
    fi->_inTime = 0; // Interpolation is off.
    fi->_timer = 0;
    fi->_gotoSkip = false;
    fi->_gotoEnd = false;
    fi->_skipNext = false;

    fi->_waitingText = 0;
    fi->_waitingPic = 0;
    memset(fi->_gotoTarget, 0, sizeof(fi->_gotoTarget));

    clearEventHandlers(fi);

    // Any hooks?
    Plug_DoHook(HOOK_FINALE_SCRIPT_BEGIN, fi->_id, 0);
    }
}

void FinaleInterpreter_ReleaseScript(finaleinterpreter_t* fi)
{
    assert(fi);
    if(!fi->flags.stopped)
    {
        stopScript(fi);
    }
    clearEventHandlers(fi);
    releaseScript(fi);
}

void FinaleInterpreter_Resume(finaleinterpreter_t* fi)
{
    assert(fi);
    if(!fi->flags.suspended)
        return;
    fi->flags.suspended = false;
    FIPage_Pause(fi->_pages[PAGE_PICS], false);
    FIPage_Pause(fi->_pages[PAGE_TEXT], false);
    // Do we need to unhide any pages?
    if(fi->_cmdExecuted)
    {
        FIPage_MakeVisible(fi->_pages[PAGE_PICS], true);
        FIPage_MakeVisible(fi->_pages[PAGE_TEXT], true);
    }
}

void FinaleInterpreter_Suspend(finaleinterpreter_t* fi)
{
    assert(fi);
    if(fi->flags.suspended)
        return;
    fi->flags.suspended = true;
    // While suspended, all pages will be paused and hidden.
    FIPage_Pause(fi->_pages[PAGE_PICS], true);
    FIPage_MakeVisible(fi->_pages[PAGE_PICS], false);
    FIPage_Pause(fi->_pages[PAGE_TEXT], true);
    FIPage_MakeVisible(fi->_pages[PAGE_TEXT], false);
}

boolean FinaleInterpreter_IsMenuTrigger(finaleinterpreter_t* fi)
{
    assert(fi);
    return (fi->flags.show_menu != 0);
}

boolean FinaleInterpreter_IsSuspended(finaleinterpreter_t* fi)
{
    assert(fi);
    return (fi->flags.suspended != 0);
}

void FinaleInterpreter_AllowSkip(finaleinterpreter_t* fi, boolean yes)
{
    assert(fi);
    fi->flags.can_skip = yes;
}

boolean FinaleInterpreter_CanSkip(finaleinterpreter_t* fi)
{
    assert(fi);
    return (fi->flags.can_skip != 0);
}

boolean FinaleInterpreter_CommandExecuted(finaleinterpreter_t* fi)
{
    assert(fi);
    return fi->_cmdExecuted;
}

static boolean runTic(finaleinterpreter_t* fi)
{
    ddhook_finale_script_ticker_paramaters_t params;
    memset(&params, 0, sizeof(params));
    params.runTick = true;
    params.canSkip = FinaleInterpreter_CanSkip(fi);
    Plug_DoHook(HOOK_FINALE_SCRIPT_TICKER, fi->_id, &params);
    return params.runTick;
}

boolean FinaleInterpreter_RunTic(finaleinterpreter_t* fi)
{
    assert(fi);

    if(fi->flags.stopped || fi->flags.suspended)
        return false;

    fi->_timer++;

    if(!runTic(fi))
        return false;

    // If waiting do not execute commands.
    if(fi->_wait && --fi->_wait)
        return false;

    // If paused there is nothing further to do.
    if(fi->flags.paused)
        return false;

    // If waiting on a text to finish typing, do nothing.
    if(fi->_waitingText && fi->_waitingText->type == FI_TEXT)
    {
        if(!((fidata_text_t*)fi->_waitingText)->animComplete)
            return false;

        fi->_waitingText = NULL;
    }

    // Waiting for an animation to reach its end?
    if(fi->_waitingPic && fi->_waitingPic->type == FI_PIC)
    {
        if(!((fidata_pic_t*)fi->_waitingPic)->animComplete)
            return false;

        fi->_waitingPic = NULL;
    }

    // Execute commands until a wait time is set or we reach the end of
    // the script. If the end is reached, the finale really ends (terminates).
    {int last = 0;
    while(!fi->_gotoEnd && !fi->_wait && !fi->_waitingText && !fi->_waitingPic && !last)
        last = !executeNextCommand(fi);
    return (fi->_gotoEnd || last);}
}

boolean FinaleInterpreter_SkipToMarker(finaleinterpreter_t* fi, const char* marker)
{
    assert(fi && marker);

    if(!marker[0])
        return false;

    memset(fi->_gotoTarget, 0, sizeof(fi->_gotoTarget));
    strncpy(fi->_gotoTarget, marker, sizeof(fi->_gotoTarget) - 1);

    // Start skipping until the marker is found.
    fi->_gotoSkip = true;

    // Stop any waiting.
    fi->_wait = 0;
    fi->_waitingText = NULL;
    fi->_waitingPic = NULL;

    // Rewind the script so we can jump anywhere.
    fi->_cp = fi->_script;
    return true;
}

boolean FinaleInterpreter_Skip(finaleinterpreter_t* fi)
{
    assert(fi);

    // Stop waiting for objects.
    fi->_waitingText = NULL;
    fi->_waitingPic = NULL;
    if(fi->flags.paused)
    {
        fi->flags.paused = false;
        fi->_wait = 0;
        return true;
    }

    if(fi->flags.can_skip)
    {
        fi->_skipping = true; // Start skipping ahead.
        fi->_wait = 0;
        return true;
    }

    return (fi->flags.eat_events != 0);
}

int FinaleInterpreter_Responder(finaleinterpreter_t* fi, const ddevent_t* ev)
{
    assert(fi);

    if(isClient)
        return false;

    if(fi->flags.suspended)
        return false;

    // During the first ~second disallow all events/skipping.
    if(fi->_timer < 20)
        return false;

    // Any handlers for this event?
    if(IS_KEY_DOWN(ev))
    {
        fi_handler_t* h;
        if((h = findEventHandler(fi, ev)))
        {
            FinaleInterpreter_SkipToMarker(fi, h->marker);
            // We'll never eat up events.
            if(IS_TOGGLE_UP(ev))
                return false;
            return (fi->flags.eat_events != 0);
        }
    }

    // If we can't skip, there'fi no interaction of any kind.
    if(!fi->flags.can_skip && !fi->flags.paused)
        return false;

    // We are only interested in key/button down presses.
    if(!IS_TOGGLE_DOWN(ev))
        return false;

    // Servers tell clients to skip.
    Sv_Finale(FINF_SKIP, 0);
    return FinaleInterpreter_Skip(fi);
}

DEFFC(Do)
{   // This command is called even when (cond)skipping.
    if(!fi->_skipNext)
        return;

    // A conditional skip has been issued.
    // We'll go into DO-skipping mode. skipnext won't be cleared
    // until the matching semicolon is found.
    fi->_doLevel++;
}

DEFFC(End)
{
    fi->_gotoEnd = true;
}

DEFFC(BGFlat)
{
    changePageBackground(fi->_pages[PAGE_PICS], Materials_ToMaterial(Materials_CheckNumForName(OP_CSTRING(0), MN_FLATS)));
}

DEFFC(BGTexture)
{
    changePageBackground(fi->_pages[PAGE_PICS], Materials_ToMaterial(Materials_CheckNumForName(OP_CSTRING(0), MN_TEXTURES)));
}

DEFFC(NoBGMaterial)
{
    changePageBackground(fi->_pages[PAGE_PICS], 0);
}

DEFFC(InTime)
{
    fi->_inTime = FRACSECS_TO_TICKS(OP_FLOAT(0));
}

DEFFC(Tic)
{
    fi->_wait = 1;
}

DEFFC(Wait)
{
    fi->_wait = FRACSECS_TO_TICKS(OP_FLOAT(0));
}

DEFFC(WaitText)
{
    fi->_waitingText = getObject(fi, FI_TEXT, OP_CSTRING(0));
}

DEFFC(WaitAnim)
{
    fi->_waitingPic = getObject(fi, FI_PIC, OP_CSTRING(0));
}

DEFFC(Color)
{
    FIPage_SetBackgroundTopColor(fi->_pages[PAGE_PICS], OP_FLOAT(0), OP_FLOAT(1), OP_FLOAT(2), fi->_inTime);
    FIPage_SetBackgroundBottomColor(fi->_pages[PAGE_PICS], OP_FLOAT(0), OP_FLOAT(1), OP_FLOAT(2), fi->_inTime);
}

DEFFC(ColorAlpha)
{
    FIPage_SetBackgroundTopColorAndAlpha(fi->_pages[PAGE_PICS], OP_FLOAT(0), OP_FLOAT(1), OP_FLOAT(2), OP_FLOAT(3), fi->_inTime);
    FIPage_SetBackgroundBottomColorAndAlpha(fi->_pages[PAGE_PICS], OP_FLOAT(0), OP_FLOAT(1), OP_FLOAT(2), OP_FLOAT(3), fi->_inTime);
}

DEFFC(Pause)
{
    fi->flags.paused = true;
    fi->_wait = 1;
}

DEFFC(CanSkip)
{
    fi->flags.can_skip = true;
}

DEFFC(NoSkip)
{
    fi->flags.can_skip = false;
}

DEFFC(SkipHere)
{
    fi->_skipping = false;
}

DEFFC(Events)
{
    fi->flags.eat_events = true;
}

DEFFC(NoEvents)
{
    fi->flags.eat_events = false;
}

DEFFC(OnKey)
{
    ddevent_t ev;

    // Construct a template event for this handler.
    memset(&ev, 0, sizeof(ev));
    ev.device = IDEV_KEYBOARD;
    ev.type = E_TOGGLE;
    ev.toggle.id = DD_GetKeyCode(OP_CSTRING(0));
    ev.toggle.state = ETOG_DOWN;

    // First, try to find an existing handler.
    if(findEventHandler(fi, &ev))
        return;
    // Allocate and attach another.
    createEventHandler(fi, &ev, OP_CSTRING(1));
}

DEFFC(UnsetKey)
{
    ddevent_t ev;
    fi_handler_t* h;

    // Construct a template event for what we want to "unset".
    memset(&ev, 0, sizeof(ev));
    ev.device = IDEV_KEYBOARD;
    ev.type = E_TOGGLE;
    ev.toggle.id = DD_GetKeyCode(OP_CSTRING(0));
    ev.toggle.state = ETOG_DOWN;

    if((h = findEventHandler(fi, &ev)))
    {
        destroyEventHandler(fi, h);
    }
}

DEFFC(If)
{
    const char* token = OP_CSTRING(0);
    boolean val = false;

    // Built-in conditions.
    if(!stricmp(token, "netgame"))
    {
        val = netGame;
    }
    else if(!strnicmp(token, "mode:", 5))
    {
        if(!DD_IsNullGameInfo(DD_GameInfo()))
            val = !stricmp(token + 5, Str_Text(GameInfo_IdentityKey(DD_GameInfo())));
        else
            val = 0;
    }
    // Any hooks?
    else if(Plug_CheckForHook(HOOK_FINALE_EVAL_IF))
    {
        ddhook_finale_script_evalif_paramaters_t p;
        memset(&p, 0, sizeof(p));
        p.token = token;
        p.returnVal = 0;
        if(Plug_DoHook(HOOK_FINALE_EVAL_IF, fi->_id, (void*) &p))
            val = p.returnVal;
    }
    else
    {
        Con_Message("FIC_If: Unknown condition '%s'.\n", token);
    }

    // Skip the next command if the value is false.
    fi->_skipNext = !val;
}

DEFFC(IfNot)
{
    FIC_If(cmd, ops, fi);
    fi->_skipNext = !fi->_skipNext;
}

DEFFC(Else)
{   // The only time the ELSE condition does not skip is immediately after a skip.
    fi->_skipNext = !fi->_lastSkipped;
}

DEFFC(GoTo)
{
    FinaleInterpreter_SkipToMarker(fi, OP_CSTRING(0));
}

DEFFC(Marker)
{
    // Does it match the goto string?
    if(!stricmp(fi->_gotoTarget, OP_CSTRING(0)))
        fi->_gotoSkip = false;
}

DEFFC(Delete)
{
    if(OP_OBJECT(0))
        FI_DeleteObject(removeObjectInNamespace(&fi->_namespace, OP_OBJECT(0)));
}

DEFFC(Image)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    const char* name = OP_CSTRING(1);
    lumpnum_t lumpNum;

    FIData_PicClearAnimation(obj);

    if((lumpNum = W_CheckNumForName(name)) != -1)
    {
        FIData_PicAppendFrame(obj, PFT_RAW, -1, &lumpNum, 0, false);
    }
    else
        Con_Message("FIC_Image: Warning, missing lump '%s'.\n", name);
}

DEFFC(ImageAt)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    float x = OP_FLOAT(1);
    float y = OP_FLOAT(2);
    const char* name = OP_CSTRING(3);
    lumpnum_t lumpNum;

    AnimatorVector3_Init(obj->pos, x, y, 0);
    FIData_PicClearAnimation(obj);

    if((lumpNum = W_CheckNumForName(name)) != -1)
    {
        FIData_PicAppendFrame(obj, PFT_RAW, -1, &lumpNum, 0, false);
    }
    else
        Con_Message("FIC_ImageAt: Warning, missing lump '%s'.\n", name);
}

static DGLuint loadGraphics(const char* name, gfxmode_t mode, int useMipmap, boolean clamped, int otherFlags)
{
    return GL_PrepareExtTexture(name, mode, useMipmap,
                                GL_LINEAR, GL_LINEAR, 0 /*no anisotropy*/,
                                clamped? GL_CLAMP_TO_EDGE : GL_REPEAT,
                                clamped? GL_CLAMP_TO_EDGE : GL_REPEAT,
                                otherFlags);
}

DEFFC(XImage)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    const char* fileName = OP_CSTRING(1);
    DGLuint tex;

    FIData_PicClearAnimation(obj);

    // Load the external resource.
    if((tex = loadGraphics(fileName, LGM_NORMAL, false, true, 0)))
    {
        FIData_PicAppendFrame(obj, PFT_XIMAGE, -1, &tex, 0, false);
    }
    else
        Con_Message("FIC_XImage: Warning, missing graphic '%s'.\n", fileName);
}

DEFFC(Patch)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    const char* name = OP_CSTRING(3);
    patchid_t patch;

    AnimatorVector3_Init(obj->pos, OP_FLOAT(1), OP_FLOAT(2), 0);
    FIData_PicClearAnimation(obj);

    if((patch = R_PrecachePatch(name, NULL)) != 0)
    {
        FIData_PicAppendFrame(obj, PFT_PATCH, -1, &patch, 0, 0);
    }
    else
        Con_Message("FIC_Patch: Warning, missing Patch '%s'.\n", name);
}

DEFFC(SetPatch)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    const char* name = OP_CSTRING(1);
    patchid_t patch;

    if((patch = R_PrecachePatch(name, NULL)) != 0)
    {
        if(!((fidata_pic_t*)obj)->numFrames)
        {
            FIData_PicAppendFrame(obj, PFT_PATCH, -1, &patch, 0, false);
            return;
        }

        // Convert the first frame.
        {
        fidata_pic_frame_t* f = ((fidata_pic_t*)obj)->frames[0];
        f->type = PFT_PATCH;
        f->texRef.patch = patch;
        f->tics = -1;
        f->sound = 0;
        }
    }
    else
        Con_Message("FIC_SetPatch: Warning, missing Patch '%s'.\n", name);
}

DEFFC(ClearAnim)
{
    if(OP_OBJECT(0) && OP_OBJECT(0)->type == FI_PIC)
    {
        FIData_PicClearAnimation(OP_OBJECT(0));
    }
}

DEFFC(Anim)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    const char* name = OP_CSTRING(1);
    int tics = FRACSECS_TO_TICKS(OP_FLOAT(2));
    patchid_t patch;

    if((patch = R_PrecachePatch(name, NULL)))
    {
        FIData_PicAppendFrame(obj, PFT_PATCH, tics, &patch, 0, false);
        ((fidata_pic_t*)obj)->animComplete = false;
    }
    else
        Con_Message("FIC_Anim: Warning, Patch '%s' not found.\n", name);
}

DEFFC(AnimImage)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    const char* name = OP_CSTRING(1);
    int tics = FRACSECS_TO_TICKS(OP_FLOAT(2));
    lumpnum_t lumpNum;

    if((lumpNum = W_CheckNumForName(name)) != -1)
    {
        FIData_PicAppendFrame(obj, PFT_RAW, tics, &lumpNum, 0, false);
        ((fidata_pic_t*)obj)->animComplete = false;
    }
    else
        Con_Message("FIC_AnimImage: Warning, lump '%s' not found.\n", name);
}

DEFFC(Repeat)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    ((fidata_pic_t*)obj)->flags.looping = true;
}

DEFFC(StateAnim)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    int stateId = Def_Get(DD_DEF_STATE, OP_CSTRING(1), 0);
    int count = OP_INT(2);

    // Animate N states starting from the given one.
    ((fidata_pic_t*)obj)->animComplete = false;
    for(; count > 0 && stateId > 0; count--)
    {
        state_t* st = &states[stateId];
        spriteinfo_t sinf;

        R_GetSpriteInfo(st->sprite, st->frame & 0x7fff, &sinf);
        FIData_PicAppendFrame(obj, PFT_MATERIAL, (st->tics <= 0? 1 : st->tics), sinf.material, 0, sinf.flip);

        // Go to the next state.
        stateId = st->nextState;
    }
}

DEFFC(PicSound)
{
    fi_object_t* obj = getObject(fi, FI_PIC, OP_CSTRING(0));
    int sound = Def_Get(DD_DEF_SOUND, OP_CSTRING(1), 0);
    if(!((fidata_pic_t*)obj)->numFrames)
    {
        FIData_PicAppendFrame(obj, PFT_MATERIAL, -1, 0, sound, false);
        return;
    }
    {fidata_pic_frame_t* f = ((fidata_pic_t*)obj)->frames[((fidata_pic_t*)obj)->numFrames-1];
    f->sound = sound;
    }
}

DEFFC(ObjectOffX)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        Animator_Set(&obj->pos[0], OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(ObjectOffY)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        Animator_Set(&obj->pos[1], OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(ObjectOffZ)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        Animator_Set(&obj->pos[2], OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(ObjectRGB)
{
    fi_object_t* obj = OP_OBJECT(0);
    float rgb[3];
    if(!obj || !(obj->type == FI_TEXT || obj->type == FI_PIC))
        return;
    rgb[CR] = OP_FLOAT(1);
    rgb[CG] = OP_FLOAT(2);
    rgb[CB] = OP_FLOAT(3);
    switch(obj->type)
    {
    default: Con_Error("FinaleInterpreter::FIC_ObjectRGB: Unknown type %i.", (int) obj->type);
    case FI_TEXT:
        {
        fidata_text_t* t = (fidata_text_t*)obj;
        AnimatorVector3_Set(t->color, rgb[CR], rgb[CG], rgb[CB], fi->_inTime);
        break;
        }
    case FI_PIC:
        {
        fidata_pic_t* p = (fidata_pic_t*)obj;
        AnimatorVector3_Set(p->color, rgb[CR], rgb[CG], rgb[CB], fi->_inTime);
        // This affects all the colors.
        AnimatorVector3_Set(p->otherColor, rgb[CR], rgb[CG], rgb[CB], fi->_inTime);
        AnimatorVector3_Set(p->edgeColor, rgb[CR], rgb[CG], rgb[CB], fi->_inTime);
        AnimatorVector3_Set(p->otherEdgeColor, rgb[CR], rgb[CG], rgb[CB], fi->_inTime);
        break;
        }
    }
}

DEFFC(ObjectAlpha)
{
    fi_object_t* obj = OP_OBJECT(0);
    float alpha;
    if(!obj || !(obj->type == FI_TEXT || obj->type == FI_PIC))
        return;
    alpha = OP_FLOAT(1);
    switch(obj->type)
    {
    default: Con_Error("FinaleInterpreter::FIC_ObjectAlpha: Unknown type %i.", (int) obj->type);
    case FI_TEXT:
        {
        fidata_text_t* t = (fidata_text_t*)obj;
        Animator_Set(&t->color[3], alpha, fi->_inTime);
        break;
        }
    case FI_PIC:
        {
        fidata_pic_t* p = (fidata_pic_t*)obj;
        Animator_Set(&p->color[3], alpha, fi->_inTime);
        Animator_Set(&p->otherColor[3], alpha, fi->_inTime);
        break;
        }
    }
}

DEFFC(ObjectScaleX)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        Animator_Set(&obj->scale[0], OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(ObjectScaleY)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        Animator_Set(&obj->scale[1], OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(ObjectScaleZ)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        Animator_Set(&obj->scale[2], OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(ObjectScale)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        AnimatorVector2_Set(obj->scale, OP_FLOAT(1), OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(ObjectScaleXY)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        AnimatorVector2_Set(obj->scale, OP_FLOAT(1), OP_FLOAT(2), fi->_inTime);
    }
}

DEFFC(ObjectScaleXYZ)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        AnimatorVector3_Set(obj->scale, OP_FLOAT(1), OP_FLOAT(2), OP_FLOAT(3), fi->_inTime);
    }
}

DEFFC(ObjectAngle)
{
    if(OP_OBJECT(0))
    {
        fi_object_t* obj = OP_OBJECT(0);
        Animator_Set(&obj->angle, OP_FLOAT(1), fi->_inTime);
    }
}

DEFFC(Rect)
{
    fidata_pic_t* obj = (fidata_pic_t*) getObject(fi, FI_PIC, OP_CSTRING(0));

    /**
     * We may be converting an existing Pic to a Rect, so re-init the expected
     * default state accordingly.
     *
     * danij: This seems rather error-prone to me. How about we turn them into
     * seperate object classes instead (Pic inheriting from Rect).
     */
    obj->animComplete = true;
    obj->flags.looping = false; // Yeah?

    AnimatorVector3_Init(obj->pos, OP_FLOAT(1), OP_FLOAT(2), 0);
    AnimatorVector3_Init(obj->scale, OP_FLOAT(3), OP_FLOAT(4), 1);

    // Default colors.
    AnimatorVector4_Init(obj->color, 1, 1, 1, 1);
    AnimatorVector4_Init(obj->otherColor, 1, 1, 1, 1);

    // Edge alpha is zero by default.
    AnimatorVector4_Init(obj->edgeColor, 1, 1, 1, 0);
    AnimatorVector4_Init(obj->otherEdgeColor, 1, 1, 1, 0);
}

DEFFC(FillColor)
{
    fi_object_t* obj = OP_OBJECT(0);
    int which = 0;
    float rgba[4];

    if(!obj || obj->type != FI_PIC)
        return;

    // Which colors to modify?
    if(!stricmp(OP_CSTRING(1), "top"))
        which |= 1;
    else if(!stricmp(OP_CSTRING(1), "bottom"))
        which |= 2;
    else
        which = 3;

    {uint i;
    for(i = 0; i < 4; ++i)
        rgba[i] = OP_FLOAT(2+i);
    }

    if(which & 1)
        AnimatorVector4_Set(((fidata_pic_t*)obj)->color, rgba[CR], rgba[CG], rgba[CB], rgba[CA], fi->_inTime);
    if(which & 2)
        AnimatorVector4_Set(((fidata_pic_t*)obj)->otherColor, rgba[CR], rgba[CG], rgba[CB], rgba[CA], fi->_inTime);
}

DEFFC(EdgeColor)
{
    fi_object_t* obj = OP_OBJECT(0);
    int which = 0;
    float rgba[4];

    if(!obj || obj->type != FI_PIC)
        return;

    // Which colors to modify?
    if(!stricmp(OP_CSTRING(1), "top"))
        which |= 1;
    else if(!stricmp(OP_CSTRING(1), "bottom"))
        which |= 2;
    else
        which = 3;

    {uint i;
    for(i = 0; i < 4; ++i)
        rgba[i] = OP_FLOAT(2+i);
    }

    if(which & 1)
        AnimatorVector4_Set(((fidata_pic_t*)obj)->edgeColor, rgba[CR], rgba[CG], rgba[CB], rgba[CA], fi->_inTime);
    if(which & 2)
        AnimatorVector4_Set(((fidata_pic_t*)obj)->otherEdgeColor, rgba[CR], rgba[CG], rgba[CB], rgba[CA], fi->_inTime);
}

DEFFC(OffsetX)
{
    FIPage_SetOffsetX(fi->_pages[PAGE_PICS], OP_FLOAT(0), fi->_inTime);
}

DEFFC(OffsetY)
{
    FIPage_SetOffsetY(fi->_pages[PAGE_PICS], OP_FLOAT(0), fi->_inTime);
}

DEFFC(Sound)
{
    int num = Def_Get(DD_DEF_SOUND, OP_CSTRING(0), NULL);
    if(num > 0)
        S_LocalSound(num, NULL);
}

DEFFC(SoundAt)
{
    int num = Def_Get(DD_DEF_SOUND, OP_CSTRING(0), NULL);
    float vol = MIN_OF(OP_FLOAT(1), 1);
    if(num > 0)
        S_LocalSoundAtVolume(num, NULL, vol);
}

DEFFC(SeeSound)
{
    int num = Def_Get(DD_DEF_MOBJ, OP_CSTRING(0), NULL);
    if(num < 0 || mobjInfo[num].seeSound <= 0)
        return;
    S_LocalSound(mobjInfo[num].seeSound, NULL);
}

DEFFC(DieSound)
{
    int num = Def_Get(DD_DEF_MOBJ, OP_CSTRING(0), NULL);
    if(num < 0 || mobjInfo[num].deathSound <= 0)
        return;
    S_LocalSound(mobjInfo[num].deathSound, NULL);
}

DEFFC(Music)
{
    S_StartMusic(OP_CSTRING(0), true);
}

DEFFC(MusicOnce)
{
    S_StartMusic(OP_CSTRING(0), false);
}

DEFFC(Filter)
{
    FIPage_SetFilterColorAndAlpha(fi->_pages[PAGE_TEXT], OP_FLOAT(0), OP_FLOAT(1), OP_FLOAT(2), OP_FLOAT(3), fi->_inTime);
}

DEFFC(Text)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    AnimatorVector3_Init(obj->pos, OP_FLOAT(1), OP_FLOAT(2), 0);
    FIData_TextCopy(obj, OP_CSTRING(3));
    ((fidata_text_t*)obj)->cursorPos = 0; // Restart the text.
}

DEFFC(TextFromDef)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    char* str;
    AnimatorVector3_Init(obj->pos, OP_FLOAT(1), OP_FLOAT(2), 0);
    if(!Def_Get(DD_DEF_TEXT, (char*)OP_CSTRING(3), &str))
        str = "(undefined)"; // Not found!
    FIData_TextCopy(obj, str);
    ((fidata_text_t*)obj)->cursorPos = 0; // Restart the text.
}

DEFFC(TextFromLump)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    int lnum;

    AnimatorVector3_Init(obj->pos, OP_FLOAT(1), OP_FLOAT(2), 0);
    lnum = W_CheckNumForName(OP_CSTRING(3));
    if(lnum < 0)
    {
        FIData_TextCopy(obj, "(not found)");
    }
    else
    {
        size_t i, incount, buflen;
        const char* data;
        char* str, *out;

        // Load the lump.
        data = W_CacheLumpNum(lnum, PU_STATIC);
        incount = W_LumpLength(lnum);
        buflen = 2 * incount + 1;
        str = calloc(1, buflen);
        for(i = 0, out = str; i < incount; ++i)
        {
            if(data[i] == '\n')
            {
                *out++ = '\\';
                *out++ = 'n';
            }
            else
            {
                *out++ = data[i];
            }
        }
        W_ChangeCacheTag(lnum, PU_CACHE);
        FIData_TextCopy(obj, str);
        free(str);
    }
    ((fidata_text_t*)obj)->cursorPos = 0; // Restart.
}

DEFFC(SetText)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    FIData_TextCopy(obj, OP_CSTRING(1));
}

DEFFC(SetTextDef)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    char* str;
    if(!Def_Get(DD_DEF_TEXT, OP_CSTRING(1), &str))
        str = "(undefined)"; // Not found!
    FIData_TextCopy(obj, str);
}

DEFFC(DeleteText)
{
    if(OP_OBJECT(0))
        FI_DeleteObject(removeObjectInNamespace(&fi->_namespace, OP_OBJECT(0)));
}

DEFFC(PredefinedTextColor)
{
    FIPage_SetPredefinedColor(fi->_pages[PAGE_TEXT], MINMAX_OF(1, OP_INT(0), 9)-1, OP_FLOAT(1), OP_FLOAT(2), OP_FLOAT(3), fi->_inTime);
}

DEFFC(TextRGB)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    AnimatorVector3_Set(((fidata_text_t*)obj)->color, OP_FLOAT(1), OP_FLOAT(2), OP_FLOAT(3), fi->_inTime);
}

DEFFC(TextAlpha)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    Animator_Set(&((fidata_text_t*)obj)->color[CA], OP_FLOAT(1), fi->_inTime);
}

DEFFC(TextOffX)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    Animator_Set(&obj->pos[0], OP_FLOAT(1), fi->_inTime);
}

DEFFC(TextOffY)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    Animator_Set(&obj->pos[1], OP_FLOAT(1), fi->_inTime);
}

DEFFC(TextCenter)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->textFlags &= ~(DTF_ALIGN_LEFT|DTF_ALIGN_RIGHT);
}

DEFFC(TextNoCenter)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->textFlags |= DTF_ALIGN_LEFT;
}

DEFFC(TextScroll)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->scrollWait = OP_INT(1);
    ((fidata_text_t*)obj)->scrollTimer = 0;
}

DEFFC(TextPos)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->cursorPos = OP_INT(1);
}

DEFFC(TextRate)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->wait = OP_INT(1);
}

DEFFC(TextLineHeight)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->lineHeight = OP_FLOAT(1);
}

DEFFC(Font)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    const char* fontName = OP_CSTRING(1);
    compositefontid_t font;
    if((font = R_CompositeFontNumForName(fontName)))
    {
        ((fidata_text_t*)obj)->font = font;
        return;
    }
    Con_Message("FIC_Font: Warning, unknown font '%s'.\n", fontName);
}

DEFFC(FontA)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->font = R_CompositeFontNumForName("a");
}

DEFFC(FontB)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    ((fidata_text_t*)obj)->font = R_CompositeFontNumForName("b");
}

DEFFC(NoMusic)
{
    // Stop the currently playing song.
    S_StopMusic();
}

DEFFC(TextScaleX)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    Animator_Set(&obj->scale[0], OP_FLOAT(1), fi->_inTime);
}

DEFFC(TextScaleY)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    Animator_Set(&obj->scale[1], OP_FLOAT(1), fi->_inTime);
}

DEFFC(TextScale)
{
    fi_object_t* obj = getObject(fi, FI_TEXT, OP_CSTRING(0));
    AnimatorVector2_Set(obj->scale, OP_FLOAT(1), OP_FLOAT(2), fi->_inTime);
}

DEFFC(PlayDemo)
{
    // While playing a demo we suspend command interpretation.
    FinaleInterpreter_Suspend(fi);

    // Start the demo.
    if(!Con_Executef(CMDS_DDAY, true, "playdemo \"%s\"", OP_CSTRING(0)))
    {   // Demo playback failed. Here we go again...
        FinaleInterpreter_Resume(fi);
    }
}

DEFFC(Command)
{
    Con_Executef(CMDS_SCRIPT, false, OP_CSTRING(0));
}

DEFFC(ShowMenu)
{
    fi->flags.show_menu = true;
}

DEFFC(NoShowMenu)
{
    fi->flags.show_menu = false;
}
