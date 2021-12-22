#include "OTRDisplayListFactory.h"

namespace OtrLib
{
    OTRDisplayList* OTRDisplayListFactory::ReadDisplayList(BinaryReader* reader)
    {
        OTRDisplayList* dl = new OTRDisplayList();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
        {
            OTRDisplayListV0 otrDL = OTRDisplayListV0();
            otrDL.ParseFileBinary(reader, dl);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return dl;

    }
}