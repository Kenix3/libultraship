#include "OTRArchive.h"

namespace OtrLib
{
    void OTRArchiveV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
    {
        //OTRResourceFile::ParseFileBinary(reader, res);

        OTRArchive* archive = (OTRArchive*)res;

        loadType = (OTRArchiveEntryType)reader->ReadByte();
        rootEntry = new OTRArchiveV0Entry(reader);

        archive->loadType = loadType;
        archive->rootEntry = rootEntry->ToArchiveEntry();
    }

    void OTRArchiveV0::WriteFileBinary(BinaryWriter* writer, OTRResource* res)
    {
        OTRResourceFile::WriteFileBinary(writer, res);

        OTRArchive* archive = (OTRArchive*)res;

        
    }

    OTRArchiveV0Entry::OTRArchiveV0Entry(BinaryReader* reader)
    {
        name = reader->ReadString();
        entryType = (OTRArchiveEntryType)reader->ReadByte();

        if (entryType == OTRArchiveEntryType::File)
        {
            uint32_t contentAddr = reader->ReadUInt32();
            contentSize = reader->ReadUInt32();
            content = new char[contentSize];

            uint32_t tempOffset = reader->GetBaseAddress();
            reader->Seek(contentAddr, SeekOffsetType::Start);

            reader->Read(content, contentSize);
            reader->Seek(tempOffset, SeekOffsetType::Start);
        }

        //archiveEntry->children = nullptr;
        uint16_t numEntries = reader->ReadUInt16();

        for (int i = 0; i < numEntries; i++)
        {
            children.push_back(new OTRArchiveV0Entry(reader));
        }
    }

    OTRArchiveEntry* OTRArchiveV0Entry::ToArchiveEntry()
    {
        return new OTRArchiveEntry();
    }
}