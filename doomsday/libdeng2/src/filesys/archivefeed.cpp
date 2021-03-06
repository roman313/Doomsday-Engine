/** @file archivefeed.cpp Archive Feed.
 * @ingroup fs
 *
 * @author Copyright &copy; 2009-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @author Copyright &copy; 2013 Daniel Swanson <danij@dengine.net>
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

#include "de/ArchiveFeed"
#include "de/ArchiveEntryFile"
#include "de/ByteArrayFile"
#include "de/ZipArchive"
#include "de/Writer"
#include "de/Folder"
#include "de/FS"
#include "de/Log"

namespace de {

DENG2_PIMPL(ArchiveFeed)
{
    /// File where the archive is stored (in a serialized format).
    File &file;

    /// The archive can be physically stored here, as Archive doesn't make a
    /// copy of the buffer.
    Block serializedArchive;

    Archive *arch;

    /// Mount point within the archive for this feed.
    String basePath;

    /// The feed whose archive this feed is using.
    ArchiveFeed *parentFeed;

    Instance(Public *feed, File &f) : Base(feed), file(f), arch(0), parentFeed(0)
    {
        /// @todo Observe the file for deletion.

        // If the file happens to be a byte array file, we can use it
        // directly to store the Archive.
        IByteArray *bytes = dynamic_cast<IByteArray *>(&f);

        // Open the archive.
        if(bytes)
        {
            LOG_TRACE("Source %s is a byte array") << f.description();

            arch = new ZipArchive(*bytes);
        }
        else
        {
            LOG_TRACE("Source %s is a stream") << f.description();

            // The file is just a stream, so we can't rely on the file
            // acting as the physical storage location for Archive.
            f >> serializedArchive;
            arch = new ZipArchive(serializedArchive);
        }
    }

    Instance(Public *feed, ArchiveFeed &parentFeed, String const &path)
        : Base(feed), file(parentFeed.d->file), arch(0), basePath(path), parentFeed(&parentFeed)
    {}

    ~Instance()
    {
        if(arch)
        {
            // If modified, the archive is written back to the file.
            if(arch->modified())
            {
                LOG_MSG("Updating archive in ") << file.description();

                // Make sure we have either a compressed or uncompressed version of
                // each entry in memory before destroying the source file.
                arch->cache();

                file.clear();
                Writer(file) << *arch;
            }
            else
            {
                LOG_VERBOSE("Not updating archive in %s (not changed)") << file.description();
            }
            delete arch;
        }
    }

    Archive &archive()
    {
        if(parentFeed)
        {
            return parentFeed->archive();
        }
        return *arch;
    }

    void populate(Folder &folder)
    {
        // Get a list of the files in this directory.
        Archive::Names names;
        archive().listFiles(names, basePath);

        DENG2_FOR_EACH(Archive::Names, i, names)
        {
            if(folder.has(*i))
            {
                // Already has an entry for this, skip it (wasn't pruned so it's OK).
                return;
            }

            String entry = basePath / *i;

            std::auto_ptr<ArchiveEntryFile> archFile(new ArchiveEntryFile(*i, archive(), entry));
            // Use the status of the entry within the archive.
            archFile->setStatus(archive().entryStatus(entry));

            // Create a new file that accesses this feed's archive and interpret the contents.
            File *file = folder.fileSystem().interpret(archFile.release());
            folder.add(file);

            // We will decide on pruning this.
            file->setOriginFeed(&self);

            // Include the file in the main index.
            folder.fileSystem().index(*file);
        }

        // Also populate subfolders.
        archive().listFolders(names, basePath);

        for(Archive::Names::iterator i = names.begin(); i != names.end(); ++i)
        {
            folder.fileSystem().makeFolder(folder.path() / *i, FS::InheritPrimaryFeed);
        }
    }
};

ArchiveFeed::ArchiveFeed(File &archiveFile)
    : d(new Instance(this, archiveFile))
{}

ArchiveFeed::ArchiveFeed(ArchiveFeed &parentFeed, String const &basePath)
    : d(new Instance(this, parentFeed, basePath))
{}

ArchiveFeed::~ArchiveFeed()
{
    LOG_AS("~ArchiveFeed");
    d.reset();
}

String ArchiveFeed::description() const
{
    return "archive in " + d->file.description();
}

void ArchiveFeed::populate(Folder &folder)
{
    LOG_AS("ArchiveFeed::populate");

    d->populate(folder);
}

bool ArchiveFeed::prune(File &file) const
{
    ArchiveEntryFile *entryFile = dynamic_cast<ArchiveEntryFile *>(&file);
    if(entryFile && &entryFile->archive() == &archive())
    {
        if(!archive().hasEntry(entryFile->entryPath()))
            return true; // It's gone...

        // Prune based on entry status.
        return archive().entryStatus(entryFile->entryPath()).modifiedAt !=
               file.status().modifiedAt;
    }
    return false;
}

File *ArchiveFeed::newFile(String const &name)
{
    String newEntry = d->basePath / name;
    if(archive().hasEntry(newEntry))
    {
        /// @throw AlreadyExistsError  The entry @a name already exists in the archive.
        throw AlreadyExistsError("ArchiveFeed::newFile", name + ": already exists");
    }
    // Add an empty entry.
    archive().add(newEntry, Block());
    File *file = new ArchiveEntryFile(name, archive(), newEntry);
    file->setOriginFeed(this);
    return file;
}

void ArchiveFeed::removeFile(String const &name)
{
    archive().remove(d->basePath / name);
}

Feed *ArchiveFeed::newSubFeed(String const &name)
{
    return new ArchiveFeed(*this, d->basePath / name);
}

Archive &ArchiveFeed::archive()
{
    return d->archive();
}

Archive const &ArchiveFeed::archive() const
{
    return d->archive();
}

String const &ArchiveFeed::basePath() const
{
    return d->basePath;
}

} // namespace de
