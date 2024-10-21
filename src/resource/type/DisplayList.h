#pragma once

#include <vector>
#include "resource/Resource.h"
#include "public/bridge/gfxbridge.h"
#include <libultraship/libultra/gbi.h>

namespace LUS {
class DisplayList : public Ship::Resource<Gfx> {
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
} // namespace LUS
