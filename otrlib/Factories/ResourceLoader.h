#include "../Resource.h"

namespace Ship
{
    class ResourceLoader
    {
    public:
        static Resource* LoadResource(BinaryReader* reader);
    };
}