#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"

namespace LUS {
class ResourceFactoryBinaryMatrixV0 : public ShipDK::ResourceFactoryBinary {
  public:
    std::shared_ptr<ShipDK::IResource> ReadResource(std::shared_ptr<ShipDK::File> file) override;
};
} // namespace LUS
