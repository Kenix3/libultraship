#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"

namespace Fast {
class ResourceFactoryBinaryMatrixV0 final : public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
} // namespace Fast
