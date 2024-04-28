#include "GuiTexture.h"

namespace Ship {
GuiTexture::GuiTexture() : Resource(std::shared_ptr<ResourceInitData>()) {
}

GuiTexture::~GuiTexture() {
    if (Data != nullptr) {
        stbi_image_free(Data);
    }
}

void* GuiTexture::GetPointer() {
    return Data;
}

size_t GuiTexture::GetPointerSize() {
    return DataSize * sizeof(uint8_t);
}
} // namespace Ship
