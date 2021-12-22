#include "../OTRDisplayList.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRDisplayListFactory
    {
    public:
        static OTRDisplayList* ReadDisplayList(BinaryReader* reader);
    };
}