#pragma once

#include "resource/Resource.h"
#include "libultraship/libultra/gbi.h"
#include <vector>

namespace LUS {
class Vertex : public Resource {
  public:
    using Resource::Resource;

    Vertex() : Resource(std::shared_ptr<ResourceInitData>()) {
    }

    void* GetPointer();
    size_t GetPointerSize();

    std::vector<Vtx> VertexList;
};
} // namespace LUS
