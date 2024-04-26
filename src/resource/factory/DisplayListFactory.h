#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"
#include "resource/ResourceFactoryXML.h"

namespace LUS {
class ResourceFactoryDisplayList {
  protected:
    uint32_t GetCombineLERPValue(std::string valStr);
};

class ResourceFactoryBinaryDisplayListV0 : public ResourceFactoryDisplayList, public ShipDK::ResourceFactoryBinary {
  public:
    std::shared_ptr<ShipDK::IResource> ReadResource(std::shared_ptr<ShipDK::File> file) override;
};

class ResourceFactoryXMLDisplayListV0 : public ResourceFactoryDisplayList, public ShipDK::ResourceFactoryXML {
  public:
    std::shared_ptr<ShipDK::IResource> ReadResource(std::shared_ptr<ShipDK::File> file) override;
};
} // namespace LUS
