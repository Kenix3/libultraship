#pragma once

#include "ship/resource/Resource.h"
#include "ship/resource/ResourceFactoryBinary.h"

namespace Ship {

/**
 * @brief Factory for deserializing version-0 binary GuiTexture resources from archive files.
 */
class ResourceFactoryBinaryGuiTextureV0 final : public ResourceFactoryBinary {
  public:
    /**
     * @brief Reads a GuiTexture resource from a binary archive file.
     * @param file     The archive file containing the texture data.
     * @param initData Initialization metadata for the resource.
     * @return A shared pointer to the deserialized GuiTexture resource.
     */
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file,
                                            std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship
