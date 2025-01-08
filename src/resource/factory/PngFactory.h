#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryImg.h"

namespace Fast {
class ResourceFactoryImageTexture : public Ship::ResourceFactoryImg {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file) override;
};
} // namespace Fast