#pragma once

#include "ship/resource/Resource.h"
#include "ship/resource/ResourceFactoryBinary.h"

namespace Ship {
class ResourceFactoryBinaryFontV0 final : public ResourceFactoryBinary {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file,
                                            std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship
