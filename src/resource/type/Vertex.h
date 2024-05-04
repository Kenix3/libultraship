#pragma once

#include "resource/Resource.h"
#include <vector>

union Vtx;

namespace LUS {
class Vertex : public Ship::Resource<Vtx> {
  public:
    using Resource::Resource;

    Vertex();

    Vtx* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<Vtx> VertexList;
};
} // namespace LUS
