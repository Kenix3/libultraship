#include "OTRMaterialFactory.h"

namespace OtrLib
{
    OTRMaterial* OTRMaterialFactory::ReadMaterial(BinaryReader* reader)
    {
        OTRMaterial* mat = new OTRMaterial();

        OTRVersion version = (OTRVersion)reader->ReadUInt32();
        
        switch (version)
        {
        case OTRVersion::Deckard:
        {
            OTRMaterialV0 otrMat = OTRMaterialV0();
            otrMat.ParseFileBinary(reader, mat);
        }
            break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return mat;
    }
}