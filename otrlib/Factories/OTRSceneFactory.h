#include "../OTRScene.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRSceneFactory
    {
    public:
        static OTRScene* ReadScene(BinaryReader* reader);
    };
}