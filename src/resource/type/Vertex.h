#pragma once

#include "resource/Resource.h"
#include <vector>

union F3DVtx;

namespace LUS {
class Vertex : public Resource<F3DVtx> {
  public:
    using Resource::Resource;

    Vertex();

    F3DVtx* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<F3DVtx> VertexList;
};
} // namespace LUS
