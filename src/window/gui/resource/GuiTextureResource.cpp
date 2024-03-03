#include "GuiTextureResource.h"

namespace LUS {
GuiTextureResource::GuiTextureResource() : Resource(std::shared_ptr<ResourceInitData>()) {
}

void* GuiTextureResource::GetPointer() {
    return ImageData.data();
}

size_t GuiTextureResource::GetPointerSize() {
    return ImageData.size() * sizeof(uint8_t);
}
} // namespace LUS
