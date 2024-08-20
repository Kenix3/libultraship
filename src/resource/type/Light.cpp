#include "Light.h"

namespace LUS {
LightEntry* Light::GetPointer() {
    return &mLight;
}

size_t Light::GetPointerSize() {
    return sizeof(mLight);
}
} // namespace LUS