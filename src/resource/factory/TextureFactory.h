#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"
#include "resource/readerbox/BinaryReaderBox.h"

namespace LUS {
// ResourceFactoryXMLDisplayListV0
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
