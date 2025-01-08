#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryPng.h"

namespace Fast {
class ResourceFactoryImageTexture : public Ship::ResourceFactoryPng {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file) override;
};
} // namespace Fast