#pragma once

#include "ResourceFactory.h"

namespace LUS {
class ResourceFactoryBinary : public ResourceFactory {
  protected:
    bool FileHasValidFormatAndReader(std::shared_ptr<File> file) override;
};
} // namespace LUS
