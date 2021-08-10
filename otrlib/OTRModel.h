#pragma once

#include <vector>
#include <string>
#include "OTRResource.h"
#include "Vec2f.h"
#include "Vec3f.h"
#include "Color3b.h"

namespace OtrLib
{
    enum class OTRModelType
    {
        Normal = 0,
        Flex = 1,
        Skinned = 2,
    };

    class VertexData
    {
        DataType dataType;
        uint8_t dimensionCnt;
        uint32_t entryCnt;
        uint32_t offsetToData;
    };

    class OTRModelV0 : public OTRResourceFile
    {
    public:
        // HEADER
        OTRModelType modelType;

        uint32_t numVerts;
        uint32_t numPolys;

        uint32_t vertices;
        uint32_t normals;
        uint32_t faces;
        uint32_t vertexColors;
        uint32_t uvCoords;
        uint32_t boneWeights;

        void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
    };

    struct Vertex
    {
        Vec3f pos;
        Vec3f normal;
        Color3b color;
        Vec2f uv;

        Vertex();
        Vertex(BinaryReader* reader);
    };

    class OTRModel : public OTRResource
    {
    public:
        OTRModelType modelType;

        uint32_t numVerts;
        uint32_t numPolys;

        Vertex* vertices;
        Vec2f* boneWeights;
        uint32_t* indices;
    };
}