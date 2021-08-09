#include "OTRArchiveFactory.h"

namespace OtrLib
{
    OTRArchive* OTRArchiveFactory::CreateArchive()
    {
        OTRArchive* archive = new OTRArchive();

        return archive;
    }

    OTRArchive* OTRArchiveFactory::ReadArchive(BinaryReader* reader)
    {
        OTRArchive* archive = new OTRArchive();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
        {
            OTRArchiveV0 otrArch = OTRArchiveV0();
            otrArch.ParseFileBinary(reader, archive);
        }
            break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return archive;
    }

    void OTRArchiveFactory::WriteArchive(BinaryWriter* writer, OTRArchive* archive)
    {
        OTRArchiveV0* otrArch = new OTRArchiveV0();
        otrArch->WriteFileBinary(writer, archive);
    }
};