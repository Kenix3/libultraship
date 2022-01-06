#include "OTRSceneFactory.h"

namespace OtrLib
{
    OTRScene* OTRSceneFactory::ReadScene(BinaryReader* reader)
    {
        OTRScene* scene = new OTRScene();

        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
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