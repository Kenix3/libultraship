#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"
#include "libultra/types.h"

namespace Ship {
enum class TextureType {
    Error = 0,
    RGBA32bpp = 1,
    RGBA16bpp = 2,
    Palette4bpp = 3,
    Palette8bpp = 4,
    Grayscale4bpp = 5,
    Grayscale8bpp = 6,
    GrayscaleAlpha4bpp = 7,
    GrayscaleAlpha8bpp = 8,
    GrayscaleAlpha16bpp = 9,
};

class TextureFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(std::shared_ptr<BinaryReader> reader);
};

class TextureFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
};

class Texture : public Resource {
  public:
    void* GetPointer();
    size_t GetPointerSize();

    TextureType Type;
    uint16_t Width, Height;
    uint32_t ImageDataSize;
    uint8_t* ImageData = nullptr;

    ~Texture();
};
} // namespace Ship
