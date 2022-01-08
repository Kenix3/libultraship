#include "OTRDisplayListFactory.h"

namespace OtrLib
{
    OTRDisplayList* OTRDisplayListFactory::ReadDisplayList(BinaryReader* reader)
    {
        OTRDisplayList* dl = new OTRDisplayList();

        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
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