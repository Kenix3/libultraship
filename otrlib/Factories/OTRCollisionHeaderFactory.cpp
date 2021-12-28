#include "OTRCollisionHeaderFactory.h"

namespace OtrLib
{
    OTRCollisionHeader* OTRCollisionHeaderFactory::ReadCollisionHeader(BinaryReader* reader)
    {
        OTRCollisionHeader* colHeader = new OTRCollisionHeader();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
        {
            OTRCollisionHeaderV0 otrCol = OTRCollisionHeaderV0();
            otrCol.ParseFileBinary(reader, colHeader);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return colHeader;
    }
};