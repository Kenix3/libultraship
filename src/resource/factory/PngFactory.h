#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryImg.h"

namespace Fast {
class ResourceFactoryImageTexture : public Ship::ResourceFactoryImg {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
} // namespace Fast