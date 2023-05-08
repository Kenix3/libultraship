#include "Blob.h"

namespace LUS {
void* Blob::GetPointer() {
    return Data.data();
}

size_t Blob::GetPointerSize() {
    return Data.size() * sizeof(uint8_t);
}
} // namespace LUS
