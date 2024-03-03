#pragma once

#include "resource/Resource.h"

namespace LUS {
#define RESOURCE_TYPE_GUI_TEXTURE 0x47544558 //GTEX

struct GuiTexture {
    uint32_t RendererTextureId;
    int32_t Width;
    int32_t Height;
};

class GuiTextureResource : public Resource<void> {
  public:
    using Resource::Resource;

    GuiTextureResource();

    void* GetPointer() override;
    size_t GetPointerSize() override;

    std::vector<uint8_t> ImageData;
    GuiTexture GuiTextureData;
};
}; // namespace LUS
