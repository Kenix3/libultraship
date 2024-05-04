#pragma once

#include <vector>
#include "resource/Resource.h"
#include "graphic/Fast3D/lus_gbi.h"
// LUSTODO I don't really like this. Figure out how to handle these resource types that are in GBI.h and built into LUS
// The reason Instructions can't be a vector of Gfx is because the compiler doesn't know its the same as F3DGfx.
union Gfx;

namespace LUS {
class DisplayList : public Ship::Resource<Gfx> {
  public:
    using Resource::Resource;

    DisplayList();

    Gfx* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<F3DGfx> Instructions;
};
} // namespace LUS
