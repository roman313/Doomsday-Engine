/** @file def_data.cpp
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2006-2013 Daniel Swanson <danij@dengine.net>
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
 * Doomsday Engine Definition Files
 *
 * @todo Needs to be redesigned.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "de_platform.h"
#include "de_console.h"
#include "de_misc.h"
#include "de_graphics.h"
#include "dd_def.h"
#include "def_data.h"
#include "render/sky.h"
#include <de/memory.h>

// Helper Routines -------------------------------------------------------

void* DED_NewEntries(void** ptr, ded_count_t* cnt, size_t elemSize, int count)
{
    void* np;

    cnt->num += count;
    if(cnt->num > cnt->max)
    {
        cnt->max *= 2; // Double the size of the array.
        if(cnt->num > cnt->max)
            cnt->max = cnt->num;
        *ptr = M_Realloc(*ptr, elemSize * cnt->max);
    }

    np = (char*) *ptr + (cnt->num - count) * elemSize;
    memset(np, 0, elemSize * count); // Clear the new entries.
    return np;
}

void* DED_NewEntry(void** ptr, ded_count_t* cnt, size_t elemSize)
{
    return DED_NewEntries(ptr, cnt, elemSize, 1);
}

void DED_DelEntry(int index, void** ptr, ded_count_t* cnt, size_t elemSize)
{
    if(index < 0 || index >= cnt->num) return;

    memmove((char*) *ptr + elemSize * index,
            (char*) *ptr + elemSize * (index + 1),
            elemSize * (cnt->num - index - 1));

    if(--cnt->num < cnt->max / 2)
    {
        cnt->max /= 2;
        *ptr = M_Realloc(*ptr, elemSize * cnt->max);
    }
}

void DED_DelArray(void** ptr, ded_count_t* cnt)
{
    if(*ptr) M_Free(*ptr);
    *ptr = 0;

    cnt->num = cnt->max = 0;
}

void DED_ZCount(ded_count_t* c)
{
    c->num = c->max = 0;
}

// DED Code --------------------------------------------------------------

void DED_Init(ded_t* ded)
{
	*ded = ded_t();
}

void DED_Clear(ded_t* ded)
{
    if(ded->flags)
    {
        M_Free(ded->flags);
        ded->flags = 0;
    }

    if(ded->mobjs)
    {
        M_Free(ded->mobjs);
        ded->mobjs = 0;
    }

    if(ded->states)
    {
        int i;
        for(i = 0; i < ded->count.states.num; ++i)
        {
            ded_state_t* state = &ded->states[i];
            if(state->execute) free(state->execute);
        }
        free(ded->states);
        ded->states = 0;
    }

    if(ded->sprites)
    {
        M_Free(ded->sprites);
        ded->sprites = 0;
    }

    if(ded->lights)
    {
        int i;
        for(i = 0; i < ded->count.lights.num; ++i)
        {
            ded_light_t* li = &ded->lights[i];
            if(li->up) Uri_Delete(li->up);
            if(li->down) Uri_Delete(li->down);
            if(li->sides) Uri_Delete(li->sides);
            if(li->flare) Uri_Delete(li->flare);
        }
        M_Free(ded->lights);
        ded->lights = 0;
    }

    for(uint i = 0; i < ded->models.size(); ++i)
    {
        ded_model_t* mdl = &ded->models[i];
        for(uint j = 0; j < mdl->subCount(); ++j)
        {
            ded_submodel_t* sub = &mdl->sub(j);
            if(sub->filename)     Uri_Delete(sub->filename);
            if(sub->skinFilename) Uri_Delete(sub->skinFilename);
            if(sub->shinySkin)    Uri_Delete(sub->shinySkin);
        }
    }
    ded->models.clear();

    if(ded->sounds)
    {
        int i;
        for(i = 0; i < ded->count.sounds.num; ++i)
        {
            ded_sound_t* snd = &ded->sounds[i];
            if(snd->ext) Uri_Delete(snd->ext);
        }
        M_Free(ded->sounds);
        ded->sounds = 0;
    }

    if(ded->music)
    {
        int i;
        for(i = 0; i < ded->count.music.num; ++i)
        {
            ded_music_t* song = &ded->music[i];
            if(song->path) Uri_Delete(song->path);
        }
        M_Free(ded->music);
        ded->music = 0;
    }

    if(ded->mapInfo)
    {
        int i, j;
        for(i = 0; i < ded->count.mapInfo.num; ++i)
        {
            ded_mapinfo_t* info = &ded->mapInfo[i];

            if(info->uri) Uri_Delete(info->uri);
            if(info->execute) free(info->execute);

            for(j = 0; j < NUM_SKY_LAYERS; ++j)
            {
                ded_skylayer_t* sl = &info->sky.layers[j];
                if(sl->material) Uri_Delete(sl->material);
            }

            for(j = 0; j < NUM_SKY_MODELS; ++j)
            {
                ded_skymodel_t* sm = &info->sky.models[j];
                if(sm->execute) free(sm->execute);
            }
        }
        free(ded->mapInfo);
        ded->mapInfo = 0;
    }

    if(ded->skies)
    {
        int i, j;
        for(i = 0; i < ded->count.skies.num; ++i)
        {
            ded_sky_t* sky = &ded->skies[i];
            for(j = 0; j < NUM_SKY_LAYERS; ++j)
            {
                ded_skylayer_t* sl = &sky->layers[j];
                if(sl->material) Uri_Delete(sl->material);
            }
            for(j = 0; j < NUM_SKY_MODELS; ++j)
            {
                ded_skymodel_t* sm = &sky->models[j];
                if(sm->execute) free(sm->execute);
            }
        }
        M_Free(ded->skies);
        ded->skies = 0;
    }

    if(ded->details)
    {
        int i;
        for(i = 0; i < ded->count.details.num; ++i)
        {
            ded_detailtexture_t* dtex = &ded->details[i];
            if(dtex->material1) Uri_Delete(dtex->material1);
            if(dtex->material2) Uri_Delete(dtex->material2);
            if(dtex->stage.texture) Uri_Delete(dtex->stage.texture);
        }
        M_Free(ded->details);
        ded->details = 0;
    }

    if(ded->materials)
    {
        int i, k, m;
        for(i = 0; i < ded->count.materials.num; ++i)
        {
            ded_material_t* mat = &ded->materials[i];
            if(mat->uri) Uri_Delete(mat->uri);

            for(k = 0; k < DED_MAX_MATERIAL_LAYERS; ++k)
            {
                ded_material_layer_t *layer = &mat->layers[k];
                for(m = 0; m < layer->stageCount.num; ++m)
                {
                    if(layer->stages[m].texture) Uri_Delete(layer->stages[m].texture);
                }
                M_Free(layer->stages);
            }

            for(k = 0; k < DED_MAX_MATERIAL_DECORATIONS; ++k)
            {
                ded_material_decoration_t *light = &mat->decorations[k];
                for(m = 0; m < light->stageCount.num; ++m)
                {
                    ded_decorlight_stage_t *stage = &light->stages[m];
                    if(stage->up)    Uri_Delete(stage->up);
                    if(stage->down)  Uri_Delete(stage->down);
                    if(stage->sides) Uri_Delete(stage->sides);
                    if(stage->flare) Uri_Delete(stage->flare);
                }
                M_Free(light->stages);
            }
        }
        M_Free(ded->materials);
        ded->materials = 0;
    }

    if(ded->text)
    {
        int i;
        for(i = 0; i < ded->count.text.num; ++i)
        {
            M_Free(ded->text[i].text);
        }
        M_Free(ded->text);
        ded->text = 0;
    }

    if(ded->textureEnv)
    {
        int i, j;
        for(i = 0; i < ded->count.textureEnv.num; ++i)
        {
            ded_tenviron_t* tenv = &ded->textureEnv[i];
            for(j = 0; j < tenv->count.num; ++j)
            {
                if(tenv->materials[j]) Uri_Delete(tenv->materials[j]);
            }
            M_Free(tenv->materials);
        }
        M_Free(ded->textureEnv);
        ded->textureEnv = 0;
    }

    if(ded->compositeFonts)
    {
        int i, j;
        for(i = 0; i < ded->count.compositeFonts.num; ++i)
        {
            ded_compositefont_t* cfont = &ded->compositeFonts[i];

            if(cfont->uri) Uri_Delete(cfont->uri);

            for(j = 0; j < cfont->charMapCount.num; ++j)
            {
                if(cfont->charMap[j].path) Uri_Delete(cfont->charMap[j].path);
            }
            M_Free(cfont->charMap);
        }
        M_Free(ded->compositeFonts);
        ded->compositeFonts = 0;
    }

    if(ded->values)
    {
        int i;
        for(i = 0; i < ded->count.values.num; ++i)
        {
            M_Free(ded->values[i].id);
            M_Free(ded->values[i].text);
        }
        M_Free(ded->values);
        ded->values = 0;
    }

    if(ded->decorations)
    {
        int i, j;
        for(i = 0; i < ded->count.decorations.num; ++i)
        {
            ded_decor_t *dec = &ded->decorations[i];

            if(dec->material) Uri_Delete(dec->material);

            for(j = 0; j < DED_DECOR_NUM_LIGHTS; ++j)
            {
                ded_decoration_t *light = &dec->lights[j];
                if(light->stage.up)    Uri_Delete(light->stage.up);
                if(light->stage.down)  Uri_Delete(light->stage.down);
                if(light->stage.sides) Uri_Delete(light->stage.sides);
                if(light->stage.flare) Uri_Delete(light->stage.flare);
            }
        }
        M_Free(ded->decorations);
        ded->decorations = 0;
    }

    if(ded->reflections)
    {
        int i;
        for(i = 0; i < ded->count.reflections.num; ++i)
        {
            ded_reflection_t* ref = &ded->reflections[i];

            if(ref->material) Uri_Delete(ref->material);
            if(ref->stage.texture) Uri_Delete(ref->stage.texture);
            if(ref->stage.maskTexture) Uri_Delete(ref->stage.maskTexture);
        }
        free(ded->reflections);
        ded->reflections = 0;
    }

    if(ded->groups)
    {
        int i, j;
        for(i = 0; i < ded->count.groups.num; ++i)
        {
            ded_group_t* group = &ded->groups[i];
            for(j = 0; j < group->count.num; ++j)
            {
                if(group->members[j].material) Uri_Delete(group->members[j].material);
            }
            M_Free(group->members);
        }
        M_Free(ded->groups);
        ded->groups = 0;
    }

    if(ded->sectorTypes)
    {
        M_Free(ded->sectorTypes);
        ded->sectorTypes = 0;
    }

    if(ded->lineTypes)
    {
        int i;
        for(i = 0; i < ded->count.lineTypes.num; ++i)
        {
            ded_linetype_t* lt = &ded->lineTypes[i];
            if(lt->actMaterial) Uri_Delete(lt->actMaterial);
            if(lt->deactMaterial) Uri_Delete(lt->deactMaterial);
        }
        M_Free(ded->lineTypes);
        ded->lineTypes = 0;
    }

    if(ded->ptcGens)
    {
        int i;
        for(i = 0; i < ded->count.ptcGens.num; ++i)
        {
            ded_ptcgen_t* gen = &ded->ptcGens[i];
            if(gen->map) Uri_Delete(gen->map);
            if(gen->material) Uri_Delete(gen->material);
            if(gen->stages) M_Free(gen->stages);
        }
        M_Free(ded->ptcGens);
        ded->ptcGens = 0;
    }

    if(ded->finales)
    {
        int i;
        for(i = 0; i < ded->count.finales.num; ++i)
        {
            ded_finale_t* fin = &ded->finales[i];
            if(fin->after) Uri_Delete(fin->after);
            if(fin->before) Uri_Delete(fin->before);
            if(fin->script) free(fin->script);
        }
        M_Free(ded->finales);
        ded->finales = 0;
    }
}

int DED_AddMobj(ded_t* ded, char const* idstr)
{
    ded_mobj_t* mo = (ded_mobj_t *) DED_NewEntry((void**) &ded->mobjs, &ded->count.mobjs, sizeof(ded_mobj_t));

    strcpy(mo->id, idstr);
    return mo - ded->mobjs;
}

void DED_RemoveMobj(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->mobjs, &ded->count.mobjs, sizeof(ded_mobj_t));
}

int DED_AddFlag(ded_t* ded, char const* name, char const* text, int value)
{
    ded_flag_t* fl = (ded_flag_t *) DED_NewEntry((void**) &ded->flags, &ded->count.flags, sizeof(ded_flag_t));

    strcpy(fl->id, name);
    strcpy(fl->text, text);
    fl->value = value;
    return fl - ded->flags;
}

void DED_RemoveFlag(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->flags, &ded->count.flags, sizeof(ded_flag_t));
}

int DED_AddModel(ded_t* ded, char const* spr)
{
    ded->models.push_back(ded_model_t(spr));
    return ded->models.size() - 1;
}

void DED_RemoveModel(ded_t* ded, int index)
{
    ded->models.erase(ded->models.begin() + index);
}

int DED_AddSky(ded_t* ded, char const* id)
{
    ded_sky_t* sky = (ded_sky_t *) DED_NewEntry((void**) &ded->skies, &ded->count.skies, sizeof(ded_sky_t));
    int i;

    strcpy(sky->id, id);
    sky->height = DEFAULT_SKY_HEIGHT;
    for(i = 0; i < NUM_SKY_LAYERS; ++i)
    {
        sky->layers[i].offset = DEFAULT_SKY_SPHERE_XOFFSET;
        sky->layers[i].colorLimit = DEFAULT_SKY_SPHERE_FADEOUT_LIMIT;
    }
    for(i = 0; i < NUM_SKY_MODELS; ++i)
    {
        sky->models[i].frameInterval = 1;
        sky->models[i].color[0] = 1;
        sky->models[i].color[1] = 1;
        sky->models[i].color[2] = 1;
        sky->models[i].color[3] = 1;
    }

    return sky - ded->skies;
}

void DED_RemoveSky(ded_t* ded, int index)
{
    int i;
    for(i = 0; i < NUM_SKY_LAYERS; ++i)
    {
        ded_skylayer_t* sl = &ded->skies[index].layers[i];
        if(sl->material)
            Uri_Delete(sl->material);
    }
    DED_DelEntry(index, (void **) &ded->skies, &ded->count.skies, sizeof(ded_sky_t));
}

int DED_AddState(ded_t* ded, char const* id)
{
    ded_state_t* st = (ded_state_t *) DED_NewEntry((void**) &ded->states, &ded->count.states, sizeof(ded_state_t));

    strcpy(st->id, id);
    return st - ded->states;
}

void DED_RemoveState(ded_t* ded, int index)
{
    DED_DelEntry(index, (void **) &ded->states, &ded->count.states, sizeof(ded_state_t));
}

int DED_AddSprite(ded_t* ded, char const* name)
{
    ded_sprid_t* sp = (ded_sprid_t *) DED_NewEntry((void**) &ded->sprites, &ded->count.sprites, sizeof(ded_sprid_t));

    strcpy(sp->id, name);
    return sp - ded->sprites;
}

void DED_RemoveSprite(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->sprites, &ded->count.sprites, sizeof(ded_sprid_t));
}

int DED_AddLight(ded_t* ded, char const* stateid)
{
    ded_light_t* light = (ded_light_t *) DED_NewEntry((void**) &ded->lights, &ded->count.lights, sizeof(ded_light_t));

    strcpy(light->state, stateid);
    return light - ded->lights;
}

void DED_RemoveLight(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->lights, &ded->count.lights, sizeof(ded_light_t));
}

int DED_AddMaterial(ded_t* ded, char const* uri)
{
    ded_material_t* mat = (ded_material_t *) DED_NewEntry((void**) &ded->materials, &ded->count.materials, sizeof(ded_material_t));
    if(uri) mat->uri = Uri_NewWithPath2(uri, RC_NULL);
    return mat - ded->materials;
}

int DED_AddMaterialLayerStage(ded_material_layer_t *ml)
{
    ded_material_layer_stage_t *stage = (ded_material_layer_stage_t *) DED_NewEntry((void**) &ml->stages, &ml->stageCount, sizeof(*stage));
    return stage - ml->stages;
}

int DED_AddMaterialDecorationStage(ded_material_decoration_t *li)
{
    ded_decorlight_stage_t *stage = (ded_decorlight_stage_t *) DED_NewEntry((void**) &li->stages, &li->stageCount, sizeof(*stage));

    // The color (0,0,0) means the light is not visible during this stage.
    stage->elevation = 1;
    stage->radius    = 1;

    return stage - li->stages;
}

void DED_RemoveMaterial(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->materials, &ded->count.materials, sizeof(ded_material_t));
}

int DED_AddSound(ded_t* ded, char const* id)
{
    ded_sound_t* snd = (ded_sound_t *) DED_NewEntry((void**) &ded->sounds, &ded->count.sounds, sizeof(ded_sound_t));

    strcpy(snd->id, id);
    return snd - ded->sounds;
}

void DED_RemoveSound(ded_t* ded, int index)
{
    DED_DelEntry(index, (void **) &ded->sounds, &ded->count.sounds, sizeof(ded_sound_t));
}

int DED_AddMusic(ded_t* ded, char const* id)
{
    ded_music_t* mus = (ded_music_t *) DED_NewEntry((void**) &ded->music, &ded->count.music, sizeof(ded_music_t));

    strcpy(mus->id, id);
    return mus - ded->music;
}

void DED_RemoveMusic(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->music, &ded->count.music, sizeof(ded_music_t));
}

int DED_AddMapInfo(ded_t* ded, char const* uri)
{
    ded_mapinfo_t* inf = (ded_mapinfo_t *) DED_NewEntry((void**) &ded->mapInfo, &ded->count.mapInfo, sizeof(ded_mapinfo_t));
    int i;

    if(uri) inf->uri = Uri_NewWithPath2(uri, RC_NULL);
    inf->gravity = 1;
    inf->parTime = -1; // unknown

    inf->fogColor[0] = DEFAULT_FOG_COLOR_RED;
    inf->fogColor[1] = DEFAULT_FOG_COLOR_GREEN;
    inf->fogColor[2] = DEFAULT_FOG_COLOR_BLUE;
    inf->fogDensity = DEFAULT_FOG_DENSITY;
    inf->fogStart = DEFAULT_FOG_START;
    inf->fogEnd = DEFAULT_FOG_END;

    inf->sky.height = DEFAULT_SKY_HEIGHT;
    for(i = 0; i < NUM_SKY_LAYERS; ++i)
    {
        inf->sky.layers[i].offset = DEFAULT_SKY_SPHERE_XOFFSET;
        inf->sky.layers[i].colorLimit = DEFAULT_SKY_SPHERE_FADEOUT_LIMIT;
    }
    for(i = 0; i < NUM_SKY_MODELS; ++i)
    {
        inf->sky.models[i].frameInterval = 1;
        inf->sky.models[i].color[0] = 1;
        inf->sky.models[i].color[1] = 1;
        inf->sky.models[i].color[2] = 1;
        inf->sky.models[i].color[3] = 1;
    }

    return inf - ded->mapInfo;
}

void DED_RemoveMapInfo(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->mapInfo, &ded->count.mapInfo, sizeof(ded_mapinfo_t));
}

int DED_AddText(ded_t* ded, char const* id)
{
    ded_text_t* txt = (ded_text_t *) DED_NewEntry((void**) &ded->text, &ded->count.text, sizeof(ded_text_t));

    strcpy(txt->id, id);
    return txt - ded->text;
}

void DED_RemoveText(ded_t* ded, int index)
{
    M_Free(ded->text[index].text);
    DED_DelEntry(index, (void**) &ded->text, &ded->count.text, sizeof(ded_text_t));
}

int DED_AddTextureEnv(ded_t* ded, char const* id)
{
    ded_tenviron_t* env = (ded_tenviron_t *) DED_NewEntry((void**) &ded->textureEnv, &ded->count.textureEnv, sizeof(ded_tenviron_t));

    strcpy(env->id, id);
    return env - ded->textureEnv;
}

void DED_RemoveTextureEnv(ded_t* ded, int index)
{
    { int i;
    for(i = 0; i < ded->textureEnv[index].count.num; ++i)
    {
        if(ded->textureEnv[index].materials[i])
            Uri_Delete(ded->textureEnv[index].materials[i]);
    }}
    M_Free(ded->textureEnv[index].materials);

    DED_DelEntry(index, (void**) &ded->textureEnv, &ded->count.textureEnv, sizeof(ded_tenviron_t));
}

int DED_AddCompositeFont(ded_t* ded, char const* uri)
{
    ded_compositefont_t* cfont = (ded_compositefont_t *) DED_NewEntry((void **) &ded->compositeFonts, &ded->count.compositeFonts, sizeof(ded_compositefont_t));

    if(uri) cfont->uri = Uri_NewWithPath2(uri, RC_NULL);
    return cfont - ded->compositeFonts;
}

void DED_RemoveCompositeFont(ded_t* ded, int index)
{
    ded_compositefont_t* cfont = ded->compositeFonts + index;
    int i;

    if(cfont->uri) Uri_Delete(cfont->uri);

    for(i = 0; i < cfont->charMapCount.num; ++i)
    {
        if(cfont->charMap[i].path) Uri_Delete(cfont->charMap[i].path);
    }
    M_Free(cfont->charMap);

    DED_DelEntry(index, (void**) &ded->compositeFonts, &ded->count.compositeFonts, sizeof(ded_compositefont_t));
}

int DED_AddValue(ded_t* ded, char const* id)
{
    ded_value_t* val = (ded_value_t *) DED_NewEntry((void**) &ded->values, &ded->count.values, sizeof(ded_value_t));

    if(id)
    {
        val->id = (char *) M_Malloc(strlen(id) + 1);
        strcpy(val->id, id);
    }
    return val - ded->values;
}

void DED_RemoveValue(ded_t* ded, int index)
{
    M_Free(ded->values[index].id);
    M_Free(ded->values[index].text);
    DED_DelEntry(index, (void**) &ded->values, &ded->count.values, sizeof(ded_value_t));
}

int DED_AddDetail(ded_t *ded, char const *lumpname)
{
    ded_detailtexture_t *dtl = (ded_detailtexture_t *) DED_NewEntry((void**) &ded->details, &ded->count.details, sizeof(ded_detailtexture_t));

    // Default usage is allowed with custom textures and external replacements.
    dtl->flags = DTLF_PWAD|DTLF_EXTERNAL;

    if(lumpname && lumpname[0])
    {
        dtl->stage.texture = Uri_NewWithPath2(lumpname, RC_NULL);
    }
    dtl->stage.scale = 1;
    dtl->stage.strength = 1;

    return dtl - ded->details;
}

void DED_RemoveDetail(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->details, &ded->count.details, sizeof(ded_detailtexture_t));
}

int DED_AddPtcGen(ded_t* ded, char const* state)
{
    ded_ptcgen_t* gen = (ded_ptcgen_t *) DED_NewEntry((void**) &ded->ptcGens, &ded->count.ptcGens, sizeof(ded_ptcgen_t));

    strcpy(gen->state, state);

    // Default choice (use either submodel zero or one).
    gen->subModel = -1;

    return gen - ded->ptcGens;
}

int DED_AddPtcGenStage(ded_ptcgen_t* gen)
{
    ded_ptcstage_t* stage = (ded_ptcstage_t *) DED_NewEntry((void**) &gen->stages, &gen->stageCount, sizeof(ded_ptcstage_t));

    stage->model = -1;
    stage->sound.volume = 1;
    stage->hitSound.volume = 1;

    return stage - gen->stages;
}

void DED_RemovePtcGen(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->ptcGens, &ded->count.ptcGens, sizeof(ded_ptcgen_t));
}

int DED_AddFinale(ded_t* ded)
{
    ded_finale_t* fin = (ded_finale_t *) DED_NewEntry((void**) &ded->finales, &ded->count.finales, sizeof(ded_finale_t));

    return fin - ded->finales;
}

void DED_RemoveFinale(ded_t* ded, int index)
{
    M_Free(ded->finales[index].script);
    DED_DelEntry(index, (void**) &ded->finales, &ded->count.finales, sizeof(ded_finale_t));
}

int DED_AddDecoration(ded_t* ded)
{
    ded_decor_t* decor = (ded_decor_t *) DED_NewEntry((void**) &ded->decorations, &ded->count.decorations, sizeof(ded_decor_t));
    int i;
    for(i = 0; i < DED_DECOR_NUM_LIGHTS; ++i)
    {
        // The color (0,0,0) means the light is not active.
        decor->lights[i].stage.elevation = 1;
        decor->lights[i].stage.radius    = 1;
    }

    return decor - ded->decorations;
}

void DED_RemoveDecoration(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->decorations, &ded->count.decorations, sizeof(ded_decor_t));
}

int DED_AddReflection(ded_t *ded)
{
    ded_reflection_t *ref = (ded_reflection_t *) DED_NewEntry((void **) &ded->reflections, &ded->count.reflections, sizeof(ded_reflection_t));

    // Default usage is allowed with custom textures and external replacements.
    ref->flags = REFF_PWAD|REFF_EXTERNAL;

    // Init to defaults.
    ref->stage.shininess  = 1.0f;
    ref->stage.blendMode  = BM_ADD;
    ref->stage.maskWidth  = 1.0f;
    ref->stage.maskHeight = 1.0f;

    return ref - ded->reflections;
}

void DED_RemoveReflection(ded_t *ded, int index)
{
    DED_DelEntry(index, (void **) &ded->reflections, &ded->count.reflections, sizeof(ded_reflection_t));
}

int DED_AddGroup(ded_t* ded)
{
    ded_group_t* group = (ded_group_t *) DED_NewEntry((void**) &ded->groups, &ded->count.groups, sizeof(ded_group_t));
    return group - ded->groups;
}

void DED_RemoveGroup(ded_t* ded, int index)
{
    if(ded->groups[index].members)
    {
        int i;
        for(i = 0; i < ded->groups[index].count.num; ++i)
        {
            if(ded->groups[index].members[i].material)
                Uri_Delete(ded->groups[index].members[i].material);
        }
        M_Free(ded->groups[index].members);
    }
    DED_DelEntry(index, (void**) &ded->groups, &ded->count.groups, sizeof(ded_group_t));
}

int DED_AddGroupMember(ded_group_t* grp)
{
    ded_group_member_t* memb = (ded_group_member_t *) DED_NewEntry((void**) &grp->members, &grp->count, sizeof(ded_group_member_t));

    return memb - grp->members;
}

int DED_AddSectorType(ded_t* ded, int id)
{
    ded_sectortype_t* sec = (ded_sectortype_t *) DED_NewEntry((void**) &ded->sectorTypes, &ded->count.sectorTypes, sizeof(ded_sectortype_t));

    sec->id = id;
    return sec - ded->sectorTypes;
}

void DED_RemoveSectorType(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->sectorTypes, &ded->count.sectorTypes, sizeof(ded_sectortype_t));
}

int DED_AddLineType(ded_t* ded, int id)
{
    ded_linetype_t* li = (ded_linetype_t *) DED_NewEntry((void**) &ded->lineTypes, &ded->count.lineTypes, sizeof(ded_linetype_t));

    li->id = id;
    //li->actCount = -1;
    return li - ded->lineTypes;
}

void DED_RemoveLineType(ded_t* ded, int index)
{
    DED_DelEntry(index, (void**) &ded->lineTypes, &ded->count.lineTypes, sizeof(ded_linetype_t));
}
