#include "../OTRMaterial.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    class OTRMaterialFactory
    {
    public:
        static OTRMaterial* ReadMaterial(BinaryReader* reader);
    };
}