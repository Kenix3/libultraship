#include "../OTRResource.h"

namespace OtrLib
{
    class OTRResourceLoader
    {
    public:
        static OTRResource* LoadResource(BinaryReader* reader);
    };
}