#include "Font.h"

namespace Ship {
Font::Font() : Resource(std::shared_ptr<ResourceInitData>()) {
}

Font::~Font() {
    // let ImGui free the font data
}

void* Font::GetPointer() {
    return Data;
}

size_t Font::GetPointerSize() {
    return DataSize * sizeof(char);
}
} // namespace Ship
