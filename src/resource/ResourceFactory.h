#pragma once

#include <memory>
#include "File.h"
#include "Resource.h"

namespace LUS {
class ResourceFactory {
  public:
    virtual std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) = 0;
};
} // namespace LUS
