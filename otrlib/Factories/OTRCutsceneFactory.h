#include "../OTRCutscene.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRCutsceneFactory
    {
    public:
        static OTRCutscene* ReadCutscene(BinaryReader* reader);
    };
}