#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"
#include "resource/ResourceFactoryXML.h"

namespace LUS {
class ResourceFactoryDisplayList {
  protected:
    uint32_t GetCombineLERPValue(std::string valStr);
};

class ResourceFactoryBinaryDisplayListV0 : public ResourceFactoryDisplayList, public ResourceFactoryBinary {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};

class ResourceFactoryXMLDisplayListV0 : public ResourceFactoryDisplayList, public ResourceFactoryXML {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};
} // namespace LUS
