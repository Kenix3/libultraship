#pragma once

#include "resource/Resource.h"
#include "libultra/types.h"

namespace Ship {
class Matrix : public Resource {
  public:
    void* GetPointer();
    size_t GetPointerSize();

    Mtx Matrx;
};
} // namespace Ship
