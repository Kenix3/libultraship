#include "resource/type/Texture.h"

namespace LUS {
Texture::Texture() : Resource(std::shared_ptr<ResourceInitData>()) {
}

void* Texture::GetPointer() {
    return ImageData;
}

size_t Texture::GetPointerSize() {
    return ImageDataSize;
}

Texture::~Texture() {
    if (ImageData != nullptr) {
        delete ImageData;
    }
}
} // namespace LUS
