#pragma once

#include <memory>
#include "File.h"
#include "Resource.h"

namespace Ship {
class ResourceFactory {
  public:
    virtual std::shared_ptr<Ship::IResource> ReadResource(std::shared_ptr<Ship::File> file) = 0;

  protected:
    virtual bool FileHasValidFormatAndReader(std::shared_ptr<Ship::File> file) = 0;
};
} // namespace Ship
