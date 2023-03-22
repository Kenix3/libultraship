#pragma once

#include <vector>
#include "resource/Resource.h"
#include "libultraship/libultra/gbi.h"

namespace Ship {
class DisplayList : public Resource {
  public:
    using Resource::Resource;

    void* GetPointer();
    size_t GetPointerSize();

    std::vector<Gfx> Instructions;
};
} // namespace Ship
