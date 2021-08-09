#include "../OTRMaterial.h"
#include "../Lib/BinaryReader.h"

namespace OtrLib
{
    class OTRMaterialFactory
    {
    public:
        static OTRMaterial* ReadMaterial(BinaryReader* reader);
    };
}