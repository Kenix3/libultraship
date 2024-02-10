#pragma once

#include "resource/Resource.h"
#include "resource/BinaryResourceFactory.h"
#include "resource/XMLResourceFactory.h"

namespace LUS {
class ResourceFactoryDisplayList {
  protected:
    uint32_t GetCombineLERPValue(std::string valStr);
};

class ResourceFactoryBinaryDisplayListV0 : public ResourceFactoryDisplayList, public BinaryResourceFactory {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};

class ResourceFactoryXMLDisplayListV0 : public ResourceFactoryDisplayList, public XMLResourceFactory {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;  
};
} // namespace LUS
