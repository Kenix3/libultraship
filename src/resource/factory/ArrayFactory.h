#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace LUS {
class ArrayFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(std::shared_ptr<ResourceManager> resourceMgr,
                                           std::shared_ptr<ResourceInitData> initData,
                                           std::shared_ptr<BinaryReader> reader) override;
};

class ArrayFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
};
} // namespace LUS
