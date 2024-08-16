#include "resource/type/Lights.h"
#include "libultraship/libultra/gbi.h"

namespace LUS {
Light::Light() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

Lights1* Light::GetPointer() {
    return &Lit;
}

size_t Light::GetPointerSize() {
    return sizeof(Lights1);
}
} // namespace LUS
