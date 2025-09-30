#pragma once

#include <vector>
#include "ship/resource/Resource.h"
#include "fast/public/bridge/gfxbridge.h"
#include <libultraship/libultra/gbi.h>

namespace Fast {
class DisplayList final : public Ship::Resource<Gfx> {
  public:
    using Resource::Resource;

    DisplayList();
    ~DisplayList();

    Gfx* GetPointer() override;
    size_t GetPointerSize() override;

    UcodeHandlers UCode;
    std::vector<Gfx> Instructions;
    std::vector<char*> Strings;
};
} // namespace Fast
