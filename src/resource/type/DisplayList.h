#pragma once

#include <vector>
#include "resource/Resource.h"
#include "libultraship/libultra/gbi.h"

namespace LUS {
class DisplayList : public Resource {
  public:
    using Resource::Resource;

    DisplayList();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<Gfx> Instructions;
};
} // namespace LUS
