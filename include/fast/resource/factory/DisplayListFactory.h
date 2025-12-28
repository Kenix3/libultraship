#pragma once

#include "ship/resource/Resource.h"
#include "ship/resource/ResourceFactoryBinary.h"
#include "ship/resource/ResourceFactoryXML.h"

namespace Fast {
class ResourceFactoryDisplayList {
  protected:
    uint32_t GetCombineLERPValue(const char* valStr);
};

class ResourceFactoryBinaryDisplayListV0 final : public ResourceFactoryDisplayList, public Ship::ResourceFactoryBinary {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};

class ResourceFactoryXMLDisplayListV0 final : public ResourceFactoryDisplayList, public Ship::ResourceFactoryXML {
  public:
    std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) override;
};
} // namespace Fast
