#pragma once

#include "resource/Resource.h"
#include "libultraship/libultra/types.h"
#include "libultraship/libultra/gbi.h"

namespace LUS {
class Light : public Ship::Resource<Lights1> {
  public:
    using Resource::Resource;

    Light();

    Lights1* GetPointer() override;
    size_t GetPointerSize() override;

    Lights1 Lit;
};
} // namespace LUS
