#include "Font.h"

namespace LUS {
Font::Font() : Resource(std::shared_ptr<ResourceInitData>()) {
}

void* Font::GetPointer() {
    return Data.data();
}

size_t Font::GetPointerSize() {
    return Data.size() * sizeof(char);
}
} // namespace LUS
