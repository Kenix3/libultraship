#pragma once

#include "resource/Resource.h"
#include "libultraship/libultra/gbi.h"
#include <vector>

namespace LUS {
class Vertex : public Resource {
  public:
    using Resource::Resource;

    Vertex();

    void* GetRawPointer() override;
    size_t GetPointerSize() override;

    std::vector<Vtx> VertexList;
};
} // namespace LUS
