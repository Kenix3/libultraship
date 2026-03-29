#include "fast/resource/type/Texture.h"

namespace Fast {
Texture::Texture() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

uint8_t* Texture::GetPointer() {
    return ImageData;
}

size_t Texture::GetPointerSize() {
    return ImageDataSize;
}

Texture::~Texture() {
    if (ImageData != nullptr && !mImageBuffer) {
        delete[] ImageData;
    }
}
} // namespace Fast
