#include "OTRArrayFactory.h"

namespace OtrLib
{
    OTRArray* OTRArrayFactory::ReadArray(BinaryReader* reader)
    {
        OTRArray* arr = new OTRArray();
        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
        {
            OTRArrayV0 otrArr = OTRArrayV0();
            otrArr.ParseFileBinary(reader, arr);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return arr;
    }
};