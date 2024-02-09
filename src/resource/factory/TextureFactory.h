#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace LUS {
class ResourceFactoryBinaryTextureV0 : public ResourceFactory {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<ResourceInitData> initData,
                                            std::shared_ptr<ReaderBox> readerBox) override;
};

class ResourceFactoryBinaryTextureV1 : public ResourceFactory {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<ResourceInitData> initData,
                                            std::shared_ptr<ReaderBox> readerBox) override;  
};
} // namespace LUS
