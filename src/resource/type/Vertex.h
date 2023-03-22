#pragma once

#include "resource/Resource.h"
#include "libultraship/libultra/gbi.h"
#include <vector>

namespace Ship {
class Vertex : public Resource {
  public:
    using Resource::Resource;

    void* GetPointer();
    size_t GetPointerSize();

    std::vector<Vtx> VertexList;
};
} // namespace Ship
