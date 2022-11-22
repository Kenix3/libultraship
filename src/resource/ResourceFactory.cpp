#include "ResourceFactory.h"

namespace Ship {
void ResourceVersionFactory::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::ParseFileXML(std::shared_ptr<tinyxml2::XMLElement> reader,
                                          std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::WriteFileBinary(std::shared_ptr<BinaryWriter> writer, std::shared_ptr<Resource> resource) {
}

void ResourceVersionFactory::WriteFileXML(std::shared_ptr<tinyxml2::XMLElement> writer,
                                          std::shared_ptr<Resource> resource) {
}
} // namespace Ship
