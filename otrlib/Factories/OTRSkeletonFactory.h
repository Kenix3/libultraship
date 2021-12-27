#include "../OTRSkeleton.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRSkeletonFactory
    {
    public:
        static OTRSkeleton* ReadSkeleton(BinaryReader* reader);
    };
}