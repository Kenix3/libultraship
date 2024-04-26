#pragma once

#include "ResourceFactory.h"

namespace ShipDK {
class ResourceFactoryBinary : public ResourceFactory {
  protected:
    bool FileHasValidFormatAndReader(std::shared_ptr<ShipDK::File> file) override;
};
} // namespace ShipDK
