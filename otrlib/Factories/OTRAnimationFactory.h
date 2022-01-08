#include "../OTRAnimation.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRAnimationFactory
    {
    public:
        static OTRAnimation* ReadAnimation(BinaryReader* reader);
    };
}