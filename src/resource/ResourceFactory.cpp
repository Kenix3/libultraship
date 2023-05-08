#include "ResourceFactory.h"

namespace LUS {
void ResourceVersionFactory::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::ParseFileXML(tinyxml2::XMLElement* reader, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::WriteFileBinary(std::shared_ptr<BinaryWriter> writer, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::WriteFileXML(tinyxml2::XMLElement* writer, std::shared_ptr<Resource> resource) {
}

std::shared_ptr<Resource> ResourceFactory::ReadResourceXML(std::shared_ptr<ResourceManager> resourceMgr,
                                                           std::shared_ptr<ResourceInitData> initData,
                                                           tinyxml2::XMLElement* reader) {
    return nullptr;
}
} // namespace LUS
