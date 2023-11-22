#pragma once

#include <vector>
#include "resource/Resource.h"
#include "libultraship/libultra/gbi.h"

namespace LUS {

typedef struct {
    Ambient ambient;
    Light* lights;
} Lights_X;

class ExLight final : public Resource<Lights_X> {
public:
    using Resource::Resource;

    ExLight();

    Lights_X* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<Light> lights;

    Lights_X* data = nullptr;
};
} // namespace LUS
