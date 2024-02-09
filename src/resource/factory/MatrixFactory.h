#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace LUS {
class ResourceFactoryBinaryMatrixV0 : public ResourceFactory {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<ResourceInitData> initData,
                                            std::shared_ptr<ReaderBox> readerBox) override;
};
} // namespace LUS
