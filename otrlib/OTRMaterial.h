#include "OTRResource.h"

namespace OtrLib
{
    enum class OTRMaterialCmt
    {
        Wrap = 0,
        Mirror = 1,
        Clamp = 2
    };

    class OTRShaderParam
    {
    public:
        strhash name;
        DataType dataType;
        uint64_t value;

        OTRShaderParam(BinaryReader* reader);
    };

    class OTRMaterialV0 : public OTRResourceFile
    {
    public:
        // Typical N64 Stuff
        OTRMaterialCmt cmtH, cmtV;
        uint8_t clrR, clrG, clrB, clrA, clrM, clrL;

        // Modern Stuff
        strhash shaderID;
        uint32_t shaderParamsCnt;
        uint32_t offsetToShaderEntries;

        void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
    };

    class OTRMaterial : public OTRResource
    {
    public:
        // Typical N64 Stuff
        OTRMaterialCmt cmtH, cmtV;
        uint8_t clrR, clrG, clrB, clrA, clrM, clrL;

        // Modern Stuff
        strhash shaderID;
        std::vector<OTRShaderParam*> shaderParams;
    };
}