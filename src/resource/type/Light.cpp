#include "resource/type/Light.h"

namespace LUS {
ExLight::ExLight() : Resource(std::shared_ptr<ResourceInitData>()) {}

Lights_X* ExLight::GetPointer() {
    return data;
}

size_t ExLight::GetPointerSize() {
    return sizeof(Ambient) + lights.size() * sizeof(Light);
}
} // namespace LUS
