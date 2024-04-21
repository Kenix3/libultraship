#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"

namespace LUS {
class ResourceFactoryBinaryJsonV0 : public LUS::ResourceFactoryBinary {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};
}; // namespace LUS
