#include "OTRMaterialFactory.h"

namespace OtrLib
{
    OTRMaterial* OTRMaterialFactory::ReadMaterial(BinaryReader* reader)
    {
        OTRMaterial* mat = new OTRMaterial();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
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