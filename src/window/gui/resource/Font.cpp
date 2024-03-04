#include "Font.h"

namespace LUS {
Font::Font() : Resource(std::shared_ptr<ResourceInitData>()) {
}

Font::~Font() {
    if (Data != nullptr) {
        delete Data;
    }
}

void* Font::GetPointer() {
    return Data;
}

size_t Font::GetPointerSize() {
    return DataSize * sizeof(char);
}
} // namespace LUS
