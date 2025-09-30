#pragma once

#include "ship/resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"

namespace Ship {
class ResourceFactoryBinaryGuiTextureV0 final : public ResourceFactoryBinary {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file,
                                            std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship
