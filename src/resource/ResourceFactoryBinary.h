#pragma once

#include "ResourceFactory.h"

namespace Ship {
class ResourceFactoryBinary : public ResourceFactory {
  protected:
    bool FileHasValidFormatAndReader(std::shared_ptr<File> file,
                                     std::shared_ptr<Ship::ResourceInitData> initData) override;
};
} // namespace Ship
