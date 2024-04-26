#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"
#include "resource/ResourceFactoryXML.h"

namespace LUS {
class ResourceFactoryBinaryVertexV0 : public ShipDK::ResourceFactoryBinary {
  public:
    std::shared_ptr<ShipDK::IResource> ReadResource(std::shared_ptr<ShipDK::File> file) override;
};

class ResourceFactoryXMLVertexV0 : public ShipDK::ResourceFactoryXML {
  public:
    std::shared_ptr<ShipDK::IResource> ReadResource(std::shared_ptr<ShipDK::File> file) override;
};
} // namespace LUS
