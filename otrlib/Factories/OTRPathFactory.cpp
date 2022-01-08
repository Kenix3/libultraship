#include "OTRPathFactory.h"

namespace OtrLib
{
    OTRPath* OTRPathFactory::ReadPath(BinaryReader* reader)
    {
        OTRPath* path = new OTRPath();
        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
        {
            OTRPathV0 otrPath;
            otrPath.ParseFileBinary(reader, path);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return path;
    }
};