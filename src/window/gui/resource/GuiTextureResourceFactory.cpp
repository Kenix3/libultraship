#include "window/gui/resource/GuiTextureResourceFactory.h"
#include "window/gui/resource/GuiTextureResource.h"
#include "spdlog/spdlog.h"
#include <stb/stb_image.h>
#include "graphic/Fast3D/gfx_rendering_api.h"
#include "graphic/Fast3D/gfx_pc.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryGuiTextureResourceV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto guiTextureResource = std::make_shared<GuiTextureResource>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    uint32_t dataSize = file->Buffer->size();
    guiTextureResource->ImageData.reserve(dataSize);
    for (uint32_t i = 0; i < dataSize; i++) {
        guiTextureResource->ImageData.push_back(reader->ReadUByte());
    }

    guiTextureResource->GuiTextureData.Width = 0;
    guiTextureResource->GuiTextureData.Height = 0;
    uint8_t* imgData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(guiTextureResource->ImageData.data()), dataSize,
                                            &guiTextureResource->GuiTextureData.Width, &guiTextureResource->GuiTextureData.Height, nullptr, 4);

    if (imgData == nullptr) {
        SPDLOG_ERROR("Error loading imgui texture {}", stbi_failure_reason());
        return nullptr;
    }

    // todo: figure out why this doesn't work
    // guiTextureResource->ImageData.reserve(dataSize);
    // for (uint32_t i = 0; i < dataSize; i++) {
    //     guiTextureResource->ImageData.push_back(imgData[i]);
    // }

    GfxRenderingAPI* api = gfx_get_current_rendering_api();

    // TODO: Nothing ever unloads the texture from Fast3D here.
    guiTextureResource->GuiTextureData.RendererTextureId = api->new_texture();
    api->select_texture(0, guiTextureResource->GuiTextureData.RendererTextureId);
    api->set_sampler_parameters(0, false, 0, 0);
    api->upload_texture(/* guiTextureResource->ImageData.data() */ imgData, guiTextureResource->GuiTextureData.Width, guiTextureResource->GuiTextureData.Height);

    stbi_image_free(imgData);

    return guiTextureResource;
}
} // namespace LUS
