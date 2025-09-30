#pragma once

#include <memory>
#include "File.h"
#include "Resource.h"

namespace Ship {
class ResourceFactory {
  public:
    virtual std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file,
                                                    std::shared_ptr<ResourceInitData> initData) = 0;

  protected:
    virtual bool FileHasValidFormatAndReader(std::shared_ptr<File> file,
                                             std::shared_ptr<Ship::ResourceInitData> initData) = 0;
};
} // namespace Ship
