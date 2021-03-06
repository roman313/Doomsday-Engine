/** @file texturescheme.h Texture collection subspace.
 *
 * @authors Copyright © 2010-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef LIBDENG_RESOURCE_TEXTURESCHEME_H
#define LIBDENG_RESOURCE_TEXTURESCHEME_H

#include "TextureManifest"
#include "uri.hh"
#include <de/Observers>
#include <de/PathTree>
#include <de/Error>

namespace de {

/**
 * Texture collection subspace.
 *
 * @see Textures
 * @ingroup resource
 */
class TextureScheme : DENG2_OBSERVES(TextureManifest, UniqueIdChanged),
                      DENG2_OBSERVES(TextureManifest, Deletion)
{
    typedef class TextureManifest Manifest;

public:
    /// The requested manifest could not be found in the index.
    DENG2_ERROR(NotFoundError);

    /// The specified path was not valid. @ingroup errors
    DENG2_ERROR(InvalidPathError);

    DENG2_DEFINE_AUDIENCE(ManifestDefined, void schemeManifestDefined(TextureScheme &scheme, Manifest &manifest))

    /// Minimum length of a symbolic name.
    static int const min_name_length = DENG2_URI_MIN_SCHEME_LENGTH;

    /// Manifests in the scheme are placed into a tree.
    typedef PathTreeT<Manifest> Index;

public:
    /**
     * Construct a new (empty) texture subspace scheme.
     *
     * @param symbolicName  Symbolic name of the new subspace scheme. Must
     *                      have at least @ref min_name_length characters.
     */
    explicit TextureScheme(String symbolicName);
    ~TextureScheme();

    /// @return  Symbolic name of this scheme (e.g., "ModelSkins").
    String const &name() const;

    /// @return  Total number of manifests in the scheme.
    inline int size() const { return index().size(); }

    /// @return  Total number of manifests in the scheme. Same as @ref size().
    inline int count() const { return size(); }

    /**
     * Clear all manifests in the scheme (any GL textures which have been
     * acquired for associated textures will be released).
     */
    void clear();

    /**
     * Insert a new manifest at the given @a path into the scheme.
     * If a manifest already exists at this path, the existing manifest is
     * returned.
     *
     * If any of the property values (flags, dimensions, etc...) differ from
     * that which is already defined in the pre-existing manifest, any texture
     * which is currently associated is released (any GL-textures acquired for
     * it are deleted).
     *
     * @param path          Virtual path for the resultant manifest.
     * @param flags         Texture flags property.
     * @param dimensions    Logical dimensions property.
     * @param origin        World origin offset property.
     * @param uniqueId      Unique identifier property.
     * @param resourceUri   Resource URI property.
     *
     * @return  The (possibly newly created) manifest at @a path.
     */
    Manifest &declare(Path const &path, Texture::Flags flags,
                      Vector2i const &dimensions, Vector2i const &origin,
                      int uniqueId, de::Uri const *resourceUri);

    /**
     * Determines if a manifest exists on the given @a path.
     * @return @c true if a manifest exists; otherwise @a false.
     */
    bool has(Path const &path) const;

    /**
     * Search the scheme for a manifest matching @a path.
     *
     * @return  Found manifest.
     */
    Manifest const &find(Path const &path) const;

    /// @copydoc find()
    Manifest &find(Path const &path);

    /**
     * Search the scheme for a manifest whose associated resource
     * URI matches @a uri.
     *
     * @return  Found manifest.
     */
    Manifest const &findByResourceUri(Uri const &uri) const;

    /// @copydoc findByResourceUri()
    Manifest &findByResourceUri(Uri const &uri);

    /**
     * Search the scheme for a manifest whose associated unique
     * identifier matches @a uniqueId.
     *
     * @return  Found manifest.
     */
    Manifest const &findByUniqueId(int uniqueId) const;

    /// @copydoc findByUniqueId()
    Manifest &findByUniqueId(int uniqueId);

    /**
     * Provides access to the manifest index for efficient traversal.
     */
    Index const &index() const;

protected:
    // Observes Manifest UniqueIdChanged
    void manifestUniqueIdChanged(Manifest &manifest);

    // Observes Manifest Deletion.
    void manifestBeingDeleted(Manifest const &manifest);

private:
    DENG2_PRIVATE(d)
};

} // namespace de

#endif // LIBDENG_RESOURCE_TEXTURESCHEME_H
