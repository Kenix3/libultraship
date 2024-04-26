#pragma once

#include <memory>
#include "File.h"
#include "Resource.h"

namespace ShipDK {
class ResourceFactory {
  public:
    virtual std::shared_ptr<ShipDK::IResource> ReadResource(std::shared_ptr<ShipDK::File> file) = 0;

  protected:
    virtual bool FileHasValidFormatAndReader(std::shared_ptr<ShipDK::File> file) = 0;
};
} // namespace ShipDK
