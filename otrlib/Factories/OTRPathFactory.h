#include "../OTRPath.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRPathFactory
    {
    public:
        static OTRPath* ReadPath(BinaryReader* reader);
    };
}