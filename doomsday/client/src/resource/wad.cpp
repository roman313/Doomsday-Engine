/** @file wad.cpp WAD Archive.
 *
 * @authors Copyright &copy; 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright &copy; 2006-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright &copy; 2006 Jamie Jones <jamie_jones_au@yahoo.com.au>
 * @authors Copyright &copy; 1993-1996 by id Software, Inc.
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
#include "de_filesys.h"

#include "resource/lumpcache.h"
#include "resource/wad.h"

#include <vector>
#include <cstring> // memcpy
#include <de/ByteOrder>
#include <de/Error>
#include <de/NativePath>
#include <de/PathTree>
#include <de/Log>
#include <de/memoryzone.h>

static QString invalidIndexMessage(int invalidIdx, int lastValidIdx);

namespace de {

/// The following structures are used to read data directly from WAD files.
#pragma pack(1)
typedef struct {
    char identification[4];
    int32_t lumpRecordsCount;
    int32_t lumpRecordsOffset;
} wadheader_t;

typedef struct {
    int32_t filePos;
    int32_t size;
    char name[8];
} wadlumprecord_t;
#pragma pack()

class WadFile : public File1
{
public:
    WadFile(FileHandle &hndl, String path, FileInfo const &info, File1 *container)
        : File1(hndl, path, info, container), crc_(0)
    {}

    /// @return  Name of this file.
    String const &name() const
    {
        return directoryNode().name();
    }

    /**
     * Compose an absolute URI to this file.
     *
     * @param delimiter     Delimit directory using this character.
     *
     * @return The absolute URI.
     */
    Uri composeUri(QChar delimiter = '/') const
    {
        return directoryNode().path(delimiter);
    }

    /**
     * Retrieve the directory node for this file.
     *
     * @return  Directory node for this file.
     */
    PathTree::Node const &directoryNode() const
    {
        return dynamic_cast<Wad &>(container()).lumpDirectoryNode(info_.lumpIdx);
    }

    /**
     * Read the file data into @a buffer.
     *
     * @param buffer        Buffer to read into. Must be at least large enough to
     *                      contain the whole file.
     * @param tryCache      @c true= try the lump cache first.
     *
     * @return Number of bytes read.
     *
     * @see size() or info() to determine the size of buffer needed.
     */
    size_t read(uint8_t *buffer, bool tryCache = true)
    {
        return dynamic_cast<Wad &>(container()).readLump(info_.lumpIdx, buffer, tryCache);
    }

    /**
     * Read a subsection of the file data into @a buffer.
     *
     * @param buffer        Buffer to read into. Must be at least @a length bytes.
     * @param startOffset   Offset from the beginning of the file to start reading.
     * @param length        Number of bytes to read.
     * @param tryCache      If @c true try the local data cache first.
     *
     * @return Number of bytes read.
     */
    size_t read(uint8_t *buffer, size_t startOffset, size_t length, bool tryCache = true)
    {
        return dynamic_cast<Wad &>(container()).readLump(info_.lumpIdx, buffer, startOffset, length, tryCache);
    }

    /**
     * Read this lump into the local cache.
     *
     * @return Pointer to the cached copy of the associated data.
     */
    uint8_t const *cache()
    {
        return dynamic_cast<Wad &>(container()).cacheLump(info_.lumpIdx);
    }

    /**
     * Remove a lock on the locally cached data.
     *
     * @return This instance.
     */
    WadFile &unlock()
    {
        dynamic_cast<Wad &>(container()).unlockLump(info_.lumpIdx);
        return *this;
    }

    uint crc() const { return crc_; }

    /**
     * Calculate a simple CRC for the lump.
     *
     * @note This algorithm should be replaced if the CRC is needed for anything
     *       critical/meaningful.
     *
     * @attention Calls back into the owning container instance to obtain the name.
     */
    WadFile &updateCRC()
    {
        crc_ = uint(info_.size);

        PathTree::Node const &node = directoryNode();
        String const &name = node.name();
        int const nameLen = name.length();
        for(int k = 0; k < nameLen; ++k)
        {
            crc_ += name.at(k).unicode();
        }
        return *this;
    }

private:
    uint crc_;
};

struct Wad::Instance
{
    Wad *self;

    /// Number of lump records in the archived wad.
    int arcRecordsCount;

    /// Offset to the lump record table in the archived wad.
    size_t arcRecordsOffset;

    /// Directory containing structure and info records for all lumps.
    UserDataPathTree *lumpDirectory;

    /// LUT which maps logical lump indices to PathTreeNodes.
    typedef std::vector<UserDataNode *> LumpNodeLut;
    LumpNodeLut *lumpNodeLut;

    /// Lump data cache.
    LumpCache *lumpCache;

    Instance(Wad *d, FileHandle &file, String path)
        : self(d),
          arcRecordsCount(0),
          arcRecordsOffset(0),
          lumpDirectory(0),
          lumpNodeLut(0),
          lumpCache(0)
    {
        // Seek to the start of the header.
        file.seek(0, SeekSet);

        wadheader_t hdr;
        if(!readArchiveHeader(file, hdr))
            throw Wad::FormatError("Wad::Wad", QString("File %1 does not appear to be a known WAD format").arg(path));

        arcRecordsCount  = hdr.lumpRecordsCount;
        arcRecordsOffset = hdr.lumpRecordsOffset;
    }

    ~Instance()
    {
        if(lumpDirectory)
        {
            lumpDirectory->traverse(PathTree::NoBranch, NULL, PathTree::no_hash, clearWadFileWorker);
            delete lumpDirectory;
        }

        if(lumpNodeLut) delete lumpNodeLut;
        if(lumpCache) delete lumpCache;
    }

    static int clearWadFileWorker(UserDataNode &node, void * /*parameters*/)
    {
        WadFile *lump = reinterpret_cast<WadFile *>(node.userPointer());
        if(lump)
        {
            // Detach our user data from this node.
            node.setUserPointer(0);
            delete lump;
        }
        return 0; // Continue iteration.
    }

    /// @pre @a file is positioned at the start of the header.
    static bool readArchiveHeader(FileHandle &file, wadheader_t &hdr)
    {
        size_t readBytes = file.read((uint8_t *)&hdr, sizeof(wadheader_t));
        if(!(readBytes < sizeof(wadheader_t)))
        {
            hdr.lumpRecordsCount  = littleEndianByteOrder.toNative(hdr.lumpRecordsCount);
            hdr.lumpRecordsOffset = littleEndianByteOrder.toNative(hdr.lumpRecordsOffset);
            return true;
        }
        return false;
    }

    /// @return Length of the archived lump name in characters.
    static int nameLength(wadlumprecord_t const &lrec)
    {
        int nameLen = 0;
        while(nameLen < LUMPNAME_T_LASTINDEX && lrec.name[nameLen])
        { nameLen++; }
        return nameLen;
    }

    /// Perform all translations and encodings to the archived lump name and write
    /// the result to @a normName.
    static void normalizeName(wadlumprecord_t const &lrec, ddstring_t *normName)
    {
        LOG_AS("Wad");
        if(!normName) return;

        Str_Clear(normName);
        int nameLen = nameLength(lrec);
        for(int i = 0; i < nameLen; ++i)
        {
            /// The Hexen demo on Mac uses the 0x80 on some lumps, maybe has significance?
            /// @todo Ensure that this doesn't break other IWADs. The 0x80-0xff
            ///       range isn't normally used in lump names, right??
            char ch = lrec.name[i] & 0x7f;
            Str_AppendChar(normName, ch);
        }

        // Lump names allow characters the file system does not. Therefore they
        // will be percent-encoded here and later decoded if/when necessary.
        if(!Str_IsEmpty(normName))
        {
            Str_PercentEncode(normName);
        }
        else
        {
            /// We do not consider zero-length names to be valid, so replace with
            /// with _something_.
            /// @todo fixme: Handle this more elegantly...
            Str_Set(normName, "________");
        }

        // All lumps are ordained with an extension if they don't have one.
        char const *ext = F_FindFileExtension(Str_Text(normName));
        if(!(ext && Str_Length(normName) > ext - Str_Text(normName) + 1))
        {
            Str_Append(normName, !Str_CompareIgnoreCase(normName, "DEHACKED")? ".deh" : ".lmp");
        }
    }

    void readLumpDirectory()
    {
        LOG_AS("Wad");
        if(arcRecordsCount <= 0) return;
        // Already been here?
        if(lumpDirectory) return;

        // We'll load the lump directory using one continous read into a temporary
        // local buffer before we process it into our runtime representation.
        wadlumprecord_t *arcRecords = new wadlumprecord_t[arcRecordsCount];
        self->handle_->seek(arcRecordsOffset, SeekSet);
        self->handle_->read((uint8_t *)arcRecords, arcRecordsCount * sizeof(*arcRecords));

        // Reserve a small work buffer for processing archived lump names.
        ddstring_t absPath;
        Str_Reserve(Str_Init(&absPath), LUMPNAME_T_LASTINDEX + 4/*.lmp*/);

        // Intialize the directory.
        lumpDirectory = new UserDataPathTree(PathTree::MultiLeaf);

        // Build our runtime representation from the archived lump directory.
        wadlumprecord_t const *arcRecord = arcRecords;
        for(int i = 0; i < arcRecordsCount; ++i, arcRecord++)
        {
            // Determine the name for this lump in the VFS.
            normalizeName(*arcRecord, &absPath);

            // Make it absolute.
            F_PrependBasePath(&absPath, &absPath);

            FileHandle *dummy = 0; /// @todo Fixme!
            WadFile *lump =
                new WadFile(*dummy,
                            Str_Text(&absPath),
                            FileInfo(self->lastModified(), // Inherited from the container (note recursion).
                                     i,
                                     littleEndianByteOrder.toNative(arcRecord->filePos),
                                     littleEndianByteOrder.toNative(arcRecord->size),
                                     littleEndianByteOrder.toNative(arcRecord->size)),
                            self);
            UserDataNode *node = &lumpDirectory->insert(Path(Str_Text(&absPath)));
            node->setUserPointer(lump);
        }

        Str_Free(&absPath);

        // We are finished with the temporary lump directory records.
        delete[] arcRecords;
    }

    static int buildLumpNodeLutWorker(UserDataNode &node, void *parameters)
    {
        Instance *wadInst = (Instance *)parameters;
        WadFile *lump = reinterpret_cast<WadFile *>(node.userPointer());
        DENG2_ASSERT(lump && wadInst->self->isValidIndex(lump->info().lumpIdx)); // Sanity check.
        (*wadInst->lumpNodeLut)[lump->info().lumpIdx] = &node;
        return 0; // Continue iteration.
    }

    void buildLumpNodeLut()
    {
        LOG_AS("Wad");
        // Been here already?
        if(lumpNodeLut) return;

        lumpNodeLut = new LumpNodeLut(self->lumpCount());
        if(!lumpDirectory) return;

        lumpDirectory->traverse(PathTree::NoBranch, NULL, PathTree::no_hash, buildLumpNodeLutWorker, (void*)this);
    }
};

Wad::Wad(FileHandle &hndl, String path, FileInfo const &info, File1 *container)
    : File1(hndl, path, info, container)
{
    d = new Instance(this, hndl, path);
}

Wad::~Wad()
{
    clearLumpCache();
    delete d;
}

bool Wad::isValidIndex(int lumpIdx) const
{
    return lumpIdx >= 0 && lumpIdx < lumpCount();
}

int Wad::lastIndex() const
{
    return lumpCount() - 1;
}

int Wad::lumpCount() const
{
    d->readLumpDirectory();
    return d->lumpDirectory? d->lumpDirectory->size() : 0;
}

bool Wad::empty()
{
    return !lumpCount();
}

PathTree::Node &Wad::lumpDirectoryNode(int lumpIdx) const
{
    if(!isValidIndex(lumpIdx)) throw NotFoundError("Wad::lumpDirectoryNode", invalidIndexMessage(lumpIdx, lastIndex()));
    d->buildLumpNodeLut();
    return *((*d->lumpNodeLut)[lumpIdx]);
}

File1 &Wad::lump(int lumpIdx)
{
    LOG_AS("Wad");
    if(!isValidIndex(lumpIdx)) throw NotFoundError("Wad::lump", invalidIndexMessage(lumpIdx, lastIndex()));
    d->buildLumpNodeLut();
    return *reinterpret_cast<WadFile *>((*d->lumpNodeLut)[lumpIdx]->userPointer());
}

void Wad::clearCachedLump(int lumpIdx, bool *retCleared)
{
    LOG_AS("Wad::clearCachedLump");

    if(retCleared) *retCleared = false;

    if(isValidIndex(lumpIdx))
    {
        if(d->lumpCache)
        {
            d->lumpCache->remove(lumpIdx, retCleared);
        }
        else
        {
            LOG_DEBUG("LumpCache not in use, ignoring.");
        }
    }
    else
    {
        QString msg = invalidIndexMessage(lumpIdx, lastIndex());
        LOG_DEBUG(msg + ", ignoring.");
    }
}

void Wad::clearLumpCache()
{
    LOG_AS("Wad::clearLumpCache");
    if(d->lumpCache) d->lumpCache->clear();
}

uint8_t const *Wad::cacheLump(int lumpIdx)
{
    LOG_AS("Wad::cacheLump");

    if(!isValidIndex(lumpIdx)) throw NotFoundError("Wad::cacheLump", invalidIndexMessage(lumpIdx, lastIndex()));

    WadFile const &file = reinterpret_cast<WadFile &>(lump(lumpIdx));
    LOG_TRACE("\"%s:%s\" (%u bytes%s)")
        << NativePath(composePath()).pretty()
        << NativePath(file.composePath()).pretty()
        << (unsigned long) file.info().size
        << (file.info().isCompressed()? ", compressed" : "");

    // Time to create the cache?
    if(!d->lumpCache)
    {
        d->lumpCache = new LumpCache(lumpCount());
    }

    uint8_t const *data = d->lumpCache->data(lumpIdx);
    if(data) return data;

    uint8_t * region = (uint8_t *) Z_Malloc(file.info().size, PU_APPSTATIC, 0);
    if(!region) throw Error("Wad::cacheLump", QString("Failed on allocation of %1 bytes for cache copy of lump #%2").arg(file.info().size).arg(lumpIdx));

    readLump(lumpIdx, region, false);
    d->lumpCache->insert(lumpIdx, region);

    return region;
}

void Wad::unlockLump(int lumpIdx)
{
    LOG_AS("Wad::unlockLump");
    LOG_TRACE("\"%s:%s\"") << NativePath(composePath()).pretty() << NativePath(lump(lumpIdx).composePath()).pretty();

    if(isValidIndex(lumpIdx))
    {
        if(d->lumpCache)
        {
            d->lumpCache->unlock(lumpIdx);
        }
        else
        {
            LOG_DEBUG("LumpCache not in use, ignoring.");
        }
    }
    else
    {
        QString msg = invalidIndexMessage(lumpIdx, lastIndex());
        LOG_DEBUG(msg + ", ignoring.");
    }
}

size_t Wad::readLump(int lumpIdx, uint8_t *buffer, bool tryCache)
{
    LOG_AS("Wad::readLump");
    if(!isValidIndex(lumpIdx)) return 0;
    return readLump(lumpIdx, buffer, 0, lump(lumpIdx).size(), tryCache);
}

size_t Wad::readLump(int lumpIdx, uint8_t *buffer, size_t startOffset,
    size_t length, bool tryCache)
{
    LOG_AS("Wad::readLump");
    WadFile const& file = reinterpret_cast<WadFile&>(lump(lumpIdx));

    LOG_TRACE("\"%s:%s\" (%u bytes%s) [%u +%u]")
        << NativePath(composePath()).pretty()
        << NativePath(file.composePath()).pretty()
        << (unsigned long) file.size()
        << (file.isCompressed()? ", compressed" : "")
        << startOffset
        << length;

    // Try to avoid a file system read by checking for a cached copy.
    if(tryCache)
    {
        uint8_t const *data = d->lumpCache? d->lumpCache->data(lumpIdx) : 0;
        LOG_TRACE("Cache %s on #%i") << (data? "hit" : "miss") << lumpIdx;
        if(data)
        {
            size_t readBytes = MIN_OF(file.size(), length);
            std::memcpy(buffer, data + startOffset, readBytes);
            return readBytes;
        }
    }

    handle_->seek(file.info().baseOffset + startOffset, SeekSet);
    size_t readBytes = handle_->read(buffer, length);

    /// @todo Do not check the read length here.
    if(readBytes < length)
        throw Error("Wad::readLumpSection", QString("Only read %1 of %2 bytes of lump #%3").arg(readBytes).arg(length).arg(lumpIdx));

    return readBytes;
}

uint Wad::calculateCRC()
{
    uint crc = 0;
    int const numLumps = lumpCount();
    for(int i = 0; i < numLumps; ++i)
    {
        WadFile &file = reinterpret_cast<WadFile&>(lump(i));
        file.updateCRC();
        crc += file.crc();
    }
    return crc;
}

bool Wad::recognise(FileHandle &file)
{
    wadheader_t hdr;

    // Seek to the start of the header.
    size_t initPos = file.tell();
    file.seek(0, SeekSet);

    bool readHeaderOk = Wad::Instance::readArchiveHeader(file, hdr);

    // Return the stream to its original position.
    file.seek(initPos, SeekSet);

    if(!readHeaderOk) return false;
    if(memcmp(hdr.identification, "IWAD", 4) && memcmp(hdr.identification, "PWAD", 4)) return false;
    return true;
}

} // namespace de

static QString invalidIndexMessage(int invalidIdx, int lastValidIdx)
{
    QString msg = QString("Invalid lump index %1 ").arg(invalidIdx);
    if(lastValidIdx < 0) msg += "(file is empty)";
    else                 msg += QString("(valid range: [0..%2])").arg(lastValidIdx);
    return msg;
}
