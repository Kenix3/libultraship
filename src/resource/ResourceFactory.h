#pragma once

#include <memory>
#include "readerbox/ReaderBox.h"
#include "Resource.h"

namespace LUS {
class ResourceFactory {
  public:
    virtual std::shared_ptr<IResource> ReadResource(std::shared_ptr<ResourceInitData> initData,
                                                    std::shared_ptr<ReaderBox> readerBox) = 0;
};
} // namespace LUS
