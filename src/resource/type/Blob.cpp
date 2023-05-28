#include "Blob.h"

namespace LUS {
Blob::Blob() : Resource(std::shared_ptr<ResourceInitData>()) {
}

void* Blob::GetRawPointer() {
    return Data.data();
}

size_t Blob::GetPointerSize() {
    return Data.size() * sizeof(uint8_t);
}
} // namespace LUS
