#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace Ship {

class VertexFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(uint32_t version, std::shared_ptr<BinaryReader> reader);
    std::shared_ptr<Resource> ReadResourceXML(uint32_t version, tinyxml2::XMLElement* reader);
};

class VertexFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
    void ParseFileXML(tinyxml2::XMLElement* reader, std::shared_ptr<Resource> resource) override;
};
} // namespace Ship
