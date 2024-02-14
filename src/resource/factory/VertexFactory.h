#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactoryBinary.h"
#include "resource/ResourceFactoryXML.h"

namespace LUS {
class ResourceFactoryBinaryVertexV0 : public ResourceFactoryBinary {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};

class ResourceFactoryXMLVertexV0 : public ResourceFactoryXML {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file) override;
};
} // namespace LUS
