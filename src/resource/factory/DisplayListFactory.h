#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace LUS {
class ResourceFactoryDisplayList : public ResourceFactory {
  protected:
    uint32_t GetCombineLERPValue(std::string valStr);
};

class ResourceFactoryBinaryDisplayListV0 : public ResourceFactoryDisplayList {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};

class ResourceFactoryXMLDisplayListV0 : public ResourceFactoryDisplayList {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;  
};
} // namespace LUS
