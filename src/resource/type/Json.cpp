#include "Json.h"

namespace LUS {
Json::Json() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

void* Json::GetPointer() {
    return &Data;
}

size_t Json::GetPointerSize() {
    return DataSize * sizeof(char);
}
} // namespace LUS
