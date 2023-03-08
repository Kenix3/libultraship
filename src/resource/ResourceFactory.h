#pragma once

#include <memory>
#include "binarytools/BinaryReader.h"
#include <tinyxml2.h>
#include "Resource.h"

namespace Ship {
class ResourceFactory {
  public:
    virtual std::shared_ptr<Resource> ReadResource(uint32_t version, std::shared_ptr<BinaryReader> reader) = 0;
    virtual std::shared_ptr<Resource> ReadResourceXML(uint32_t version, tinyxml2::XMLElement* reader);
};

class ResourceVersionFactory {
  public:
    virtual void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource);
    virtual void ParseFileXML(tinyxml2::XMLElement* reader, std::shared_ptr<Resource> resource);
    virtual void WriteFileBinary(std::shared_ptr<BinaryWriter> writer, std::shared_ptr<Resource> resource);
    virtual void WriteFileXML(std::shared_ptr<tinyxml2::XMLElement> writer, std::shared_ptr<Resource> resource);
};
} // namespace Ship
