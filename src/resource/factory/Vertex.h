#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"
#include "libultra/gbi.h"
#include <vector>

namespace Ship {

class VertexFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(std::shared_ptr<BinaryReader> reader);
};

class VertexFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
};

class Vertex : public Resource {
  public:
    void* GetPointer();
    size_t GetPointerSize();

    std::vector<Vtx> VertexList;
};
} // namespace Ship
