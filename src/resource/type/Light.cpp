#include "Light.h"

namespace Fast {
LightEntry* Light::GetPointer() {
    return &mLight;
}

size_t Light::GetPointerSize() {
    return sizeof(mLight);
}
} // namespace Fast