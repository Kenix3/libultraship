#include "../OTRArray.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRArrayFactory
    {
    public:
        static OTRArray* ReadArray(BinaryReader* reader);
    };
}