#include "Blob.h"

namespace Ship {
Blob::Blob() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

void* Blob::GetPointer() {
    return Data.data();
}

size_t Blob::GetPointerSize() {
    return Data.size() * sizeof(uint8_t);
}
} // namespace Ship
