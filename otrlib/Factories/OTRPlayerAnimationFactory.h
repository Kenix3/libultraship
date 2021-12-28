#include "../OTRPlayerAnimation.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRPlayerAnimationFactory
    {
    public:
        static OTRPlayerAnimation* ReadPlayerAnimation(BinaryReader* reader);
    };
}