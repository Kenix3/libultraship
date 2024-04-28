#pragma once

#include "ResourceFactory.h"

namespace Ship {
class ResourceFactoryBinary : public ResourceFactory {
  protected:
    bool FileHasValidFormatAndReader(std::shared_ptr<Ship::File> file) override;
};
} // namespace Ship
