#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"

namespace ShipDK {
class ResourceFactoryBinaryFontV0 : public ResourceFactoryBinary {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};
}; // namespace ShipDK
