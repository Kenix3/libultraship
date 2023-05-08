#pragma once

#include "resource/Resource.h"
#include "libultraship/libultra/types.h"

namespace LUS {
class Matrix : public Resource {
  public:
    using Resource::Resource;

    void* GetPointer();
    size_t GetPointerSize();

    Mtx Matrx;
};
} // namespace LUS
