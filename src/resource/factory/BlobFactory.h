#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace Ship {
class BlobFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(std::shared_ptr<BinaryReader> reader);
};

class BlobFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
};
}; // namespace Ship
