#include "../OTRSkeletonLimb.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRSkeletonLimbFactory
    {
    public:
        static OTRSkeletonLimb* ReadSkeletonLimb(BinaryReader* reader);
    };
}