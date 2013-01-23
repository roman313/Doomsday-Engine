/** @file material.cpp Logical Material.
 *
 * @authors Copyright � 2009-2013 Daniel Swanson <danij@dengine.net>
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

#include <cstring>

#include "de_base.h"
#include "def_main.h"

#include "api_map.h"
#include "audio/s_environ.h"
#include "map/r_world.h"

#include "resource/material.h"

using namespace de;

Material::Layer::Layer()
{}

Material::Layer *Material::Layer::fromDef(ded_material_layer_t &def)
{
    Layer *layer = new Layer();
    for(int i = 0; i < def.stageCount.num; ++i)
    {
        layer->stages_.push_back(&def.stages[i]);
    }
    return layer;
}

int Material::Layer::stageCount() const
{
    return stages_.count();
}

Material::Layer::Stages const &Material::Layer::stages() const
{
    return stages_;
}

Material::Decoration::Decoration()
    : patternSkip_(0, 0), patternOffset_(0, 0)
{}

Material::Decoration::Decoration(Vector2i const &_patternSkip, Vector2i const &_patternOffset)
    : patternSkip_(_patternSkip), patternOffset_(_patternOffset)
{}

Material::Decoration *Material::Decoration::fromDef(ded_material_decoration_t &def)
{
    Decoration *dec = new Decoration(Vector2i(def.patternSkip),
                                     Vector2i(def.patternOffset));
    for(int i = 0; i < def.stageCount.num; ++i)
    {
        dec->stages_.push_back(&def.stages[i]);
    }
    return dec;
}

Material::Decoration *Material::Decoration::fromDef(ded_decoration_t &def)
{
    Decoration *dec = new Decoration(Vector2i(def.patternSkip),
                                     Vector2i(def.patternOffset));
    // Only the one stage.
    dec->stages_.push_back(&def.stage);
    return dec;
}

Vector2i const &Material::Decoration::patternSkip() const
{
    return patternSkip_;
}

Vector2i const &Material::Decoration::patternOffset() const
{
    return patternOffset_;
}

int Material::Decoration::stageCount() const
{
    return stages_.count();
}

Material::Decoration::Stages const &Material::Decoration::stages() const
{
    return stages_;
}

struct Material::Instance
{
    /// Manifest derived to yield the material.
    MaterialManifest &manifest;

    /// Set of use-case/context variant instances.
    Material::Variants variants;

    /// Environment audio class.
    AudioEnvironmentClass envClass;

    /// World dimensions in map coordinate space units.
    QSize dimensions;

    /// @see materialFlags
    short flags;

    /// Layers.
    Material::Layers layers;

    /// Decorations (will be projected into the map relative to a surface).
    Material::Decorations decorations;

    /// Definition from which this material was derived (if any).
    /// @todo Refactor away -ds
    ded_material_t *def;

    Instance(MaterialManifest &_manifest)
        : manifest(_manifest), envClass(AEC_UNKNOWN), flags(0), def(0)
    {}

    ~Instance()
    {
        clearDecorations();
        clearLayers();
        clearVariants();
    }

    void clearVariants()
    {
        while(!variants.isEmpty())
        {
             delete variants.takeFirst();
        }
    }

    void clearLayers()
    {
        while(!layers.isEmpty())
        {
            delete layers.takeFirst();
        }
    }

    void clearDecorations()
    {
        while(!decorations.isEmpty())
        {
            delete decorations.takeFirst();
        }
    }
};

Material::Material(MaterialManifest &_manifest, ded_material_t &def)
    : de::MapElement(DMU_MATERIAL)
{
    d = new Instance(_manifest);
    d->flags      = def.flags;
    d->dimensions = QSize(MAX_OF(0, def.width), MAX_OF(0, def.height));

    d->def        = &def;
    for(int i = 0; i < DED_MAX_MATERIAL_LAYERS; ++i)
    {
        Material::Layer *layer = Material::Layer::fromDef(def.layers[i]);
        d->layers.push_back(layer);
    }
}

Material::~Material()
{
    delete d;
}

MaterialManifest &Material::manifest() const
{
    return d->manifest;
}

bool Material::isValid() const
{
    return !!d->def;
}

void Material::ticker(timespan_t time)
{
    DENG2_FOR_EACH(Variants, i, d->variants)
    {
        (*i)->ticker(time);
    }
}

QSize const &Material::dimensions() const
{
    return d->dimensions;
}

void Material::setDimensions(QSize const &newDimensions)
{
    if(d->dimensions != newDimensions)
    {
        d->dimensions = newDimensions;
        R_UpdateMapSurfacesOnMaterialChange(this);
    }
}

void Material::setWidth(int width)
{
    if(d->dimensions.width() != width)
    {
        d->dimensions.setWidth(width);
        R_UpdateMapSurfacesOnMaterialChange(this);
    }
}

void Material::setHeight(int height)
{
    if(d->dimensions.height() != height)
    {
        d->dimensions.setHeight(height);
        R_UpdateMapSurfacesOnMaterialChange(this);
    }
}

short Material::flags() const
{
    return d->flags;
}

void Material::setFlags(short flags)
{
    d->flags = flags;
}

bool Material::isAnimated() const
{
    // Materials cease animation once they are no longer valid.
    if(!isValid()) return false;

    DENG2_FOR_EACH_CONST(Layers, i, d->layers)
    {
        if((*i)->stageCount() > 1) return true;
    }
    return false; // Not at all.
}

bool Material::isAutoGenerated() const
{
    /// @todo fixme: We should not need a definition to determine this.
    return (d->def && d->def->autoGenerated);
}

bool Material::isDetailed() const
{
    /// @todo fixme: Determine this from our own configuration.
    return false;
}

bool Material::isShiny() const
{
    /// @todo fixme: Determine this from our own configuration.
    return false;
}

bool Material::isSkyMasked() const
{
    return 0 != (d->flags & MATF_SKYMASK);
}

bool Material::isDrawable() const
{
    return 0 == (d->flags & MATF_NO_DRAW);
}

bool Material::hasGlow() const
{
    // Invalid materials do not glow.
    if(!isValid()) return false;

    DENG2_FOR_EACH_CONST(Layers, i, d->layers)
    {
        Layer::Stages const &stages = (*i)->stages();
        DENG2_FOR_EACH_CONST(Layer::Stages, k, stages)
        {
            ded_material_layer_stage_t const &stage = **k;
            if(stage.glowStrength > .0001f) return true;
        }
    }
    return false;
}

AudioEnvironmentClass Material::audioEnvironment() const
{
    if(isDrawable()) return d->envClass;
    return AEC_UNKNOWN;
}

void Material::setAudioEnvironment(AudioEnvironmentClass envClass)
{
    d->envClass = envClass;
}

Material::Layers const &Material::layers() const
{
    return d->layers;
}

void Material::addDecoration(Material::Decoration &decor)
{
    d->decorations.push_back(&decor);
}

Material::Decorations const &Material::decorations() const
{
    return d->decorations;
}

Material::Variants const &Material::variants() const
{
    return d->variants;
}

Material::Variant *Material::chooseVariant(Material::VariantSpec const &spec, bool canCreate)
{
    DENG2_FOR_EACH_CONST(Variants, i, d->variants)
    {
        VariantSpec const &cand = (*i)->spec();
        if(cand.compare(spec))
        {
            // This will do fine.
            return *i;
        }
    }

    if(!canCreate) return 0;

    Variant *variant = new Variant(*this, spec);
    d->variants.push_back(variant);
    return variant;
}

void Material::clearVariants()
{
    d->clearVariants();
}

int Material::getProperty(setargs_t &args) const
{
    switch(args.prop)
    {
    case DMU_FLAGS: {
        short flags_ = flags();
        DMU_GetValue(DMT_MATERIAL_FLAGS, &flags_, &args, 0);
        break; }

    case DMU_WIDTH: {
        int width_ = width();
        DMU_GetValue(DMT_MATERIAL_WIDTH, &width_, &args, 0);
        break; }

    case DMU_HEIGHT: {
        int height_ = height();
        DMU_GetValue(DMT_MATERIAL_HEIGHT, &height_, &args, 0);
        break; }

    default:
        /// @throw UnknownPropertyError  The requested property does not exist.
        throw UnknownPropertyError("Material::getProperty", QString("No property %1").arg(DMU_Str(args.prop)));
    }
    return false; // Continue iteration.
}

int Material::setProperty(setargs_t const &args)
{
    /// @throw WritePropertyError  The requested property is not writable.
    throw WritePropertyError("Material::setProperty", QString("Property '%1' is not writable").arg(DMU_Str(args.prop)));
}

void Material::setDefinition(ded_material_t *def)
{
    if(d->def == def) return;
    d->def = def;
}
