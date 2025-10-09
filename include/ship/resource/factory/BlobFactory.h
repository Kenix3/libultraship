#pragma once

#include "ship/resource/Resource.h"
#include "ship/resource/ResourceFactoryBinary.h"

namespace Ship {
class ResourceFactoryBinaryBlobV0 final : public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship
