#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace LUS {
class ResourceFactoryBinaryArrayV0 : public ResourceFactory {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};
} // namespace LUS
