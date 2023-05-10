#pragma once

#include <memory>
#include "binarytools/BinaryReader.h"
#include <tinyxml2.h>
#include "Resource.h"

namespace LUS {
class ResourceManager;
class ResourceFactory {
  public:
    virtual std::shared_ptr<Resource> ReadResource(std::shared_ptr<ResourceManager> resourceMgr,
                                                   std::shared_ptr<ResourceInitData> initData,
                                                   std::shared_ptr<BinaryReader> reader) = 0;
    virtual std::shared_ptr<Resource> ReadResourceXML(std::shared_ptr<ResourceManager> resourceMgr,
                                                      std::shared_ptr<ResourceInitData> initData,
                                                      tinyxml2::XMLElement* reader);
};

class ResourceVersionFactory {
  public:
    virtual void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource);
    virtual void ParseFileXML(tinyxml2::XMLElement* reader, std::shared_ptr<Resource> resource);
    virtual void WriteFileBinary(std::shared_ptr<BinaryWriter> writer, std::shared_ptr<Resource> resource);
    virtual void WriteFileXML(tinyxml2::XMLElement* writer, std::shared_ptr<Resource> resource);
};
} // namespace LUS
