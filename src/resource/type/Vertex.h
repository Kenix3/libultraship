#pragma once

#include "resource/Resource.h"
#include <vector>

union Vtx;

namespace Fast {
class Vertex final : public Ship::Resource<Vtx> {
  public:
    using Resource::Resource;

    Vertex();

    Vtx* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<Vtx> VertexList;
};
} // namespace Fast
