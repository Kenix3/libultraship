#pragma once

#include "resource/Resource.h"
#include <stb/stb_image.h>

namespace LUS {
#define RESOURCE_TYPE_GUI_TEXTURE 0x47544558 // GTEX

struct GuiTextureMetadata {
    uint32_t RendererTextureId;
    int32_t Width;
    int32_t Height;
};

class GuiTexture : public Resource<void> {
  public:
    using Resource::Resource;

    GuiTexture();
    ~GuiTexture();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    uint8_t* Data;
    size_t DataSize;
    GuiTextureMetadata Metadata;
};
}; // namespace LUS
