#include "OTRSceneFactory.h"

namespace OtrLib
{
    OTRScene* OTRSceneFactory::ReadScene(BinaryReader* reader)
    {
        OTRScene* scene = new OTRScene();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
        {
            OTRSceneV0 otrScene = OTRSceneV0();
            otrScene.ParseFileBinary(reader, scene);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return scene;
    }
}