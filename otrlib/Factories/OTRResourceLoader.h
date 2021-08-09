#include "../OTRResource.h"

namespace OtrLib
{
    class OTRResourceLoader
    {
    public:
        OTRResource* LoadResource(BinaryReader* reader);
    };
}