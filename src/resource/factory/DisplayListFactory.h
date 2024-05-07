#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"
#include "resource/ResourceFactoryXML.h"

namespace LUS {
class ResourceFactoryDisplayList {
  protected:
    uint32_t GetCombineLERPValue(const char* valStr);
};

class ResourceFactoryBinaryDisplayListV0 : public ResourceFactoryDisplayList, public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file) override;
};

class ResourceFactoryXMLDisplayListV0 : public ResourceFactoryDisplayList, public Ship::ResourceFactoryXML {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file) override;
};
} // namespace LUS
