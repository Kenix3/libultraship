#include "window/gui/resource/GuiTextureFactory.h"
#include "window/gui/resource/GuiTexture.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryGuiTextureV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto guiTexture = std::make_shared<GuiTexture>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    guiTexture->DataSize = file->Buffer->size();
    guiTexture->Metadata.Width = 0;
    guiTexture->Metadata.Height = 0;
    guiTexture->Data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(file->Buffer->data()), guiTexture->DataSize,
                                            &guiTexture->Metadata.Width, &guiTexture->Metadata.Height, nullptr, 4);

    if (guiTexture->Data == nullptr) {
        SPDLOG_ERROR("Error loading imgui texture {}", stbi_failure_reason());
        return nullptr;
    }

    // GfxRenderingAPI* api = gfx_get_current_rendering_api();

    // // TODO: Nothing ever unloads the texture from Fast3D here.
    // guiTextureResource->GuiTextureData.RendererTextureId = api->new_texture();
    // api->select_texture(0, guiTextureResource->GuiTextureData.RendererTextureId);
    // api->set_sampler_parameters(0, false, 0, 0);
    // api->upload_texture(/* guiTextureResource->ImageData.data() */ imgData, guiTextureResource->GuiTextureData.Width, guiTextureResource->GuiTextureData.Height);

    // stbi_image_free(imgData);

    return guiTexture;
}
} // namespace LUS
