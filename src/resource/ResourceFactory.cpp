#include "ResourceFactory.h"

namespace Ship {
void ResourceVersionFactory::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::ParseFileXML(tinyxml2::XMLElement* reader, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::WriteFileBinary(std::shared_ptr<BinaryWriter> writer, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::WriteFileXML(std::shared_ptr<tinyxml2::XMLElement> writer,
                                          std::shared_ptr<Resource> resource) {
}
std::shared_ptr<Resource> ResourceFactory::ReadResourceXML(uint32_t version, tinyxml2::XMLElement* reader) {
    // return std::shared_ptr<Resource>();
    return nullptr;
}
} // namespace Ship
