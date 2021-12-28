#include "../OTRCollisionHeader.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRCollisionHeaderFactory
    {
    public:
        static OTRCollisionHeader* ReadCollisionHeader(BinaryReader* reader);
    };
}