#include "RawJson.h"

namespace LUS {
RawJson::RawJson() : Resource(std::shared_ptr<ResourceInitData>()) {
}

void* RawJson::GetPointer() {
    return &Data;
}

size_t RawJson::GetPointerSize() {
    return DataSize * sizeof(char);
}
} // namespace LUS
