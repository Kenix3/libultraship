#include "resource/factory/TextureFactory.h"
#include "resource/type/Texture.h"
#include "spdlog/spdlog.h"

namespace Ship {

std::shared_ptr<Resource> TextureFactory::ReadResource(std::shared_ptr<BinaryReader> reader) {
    auto resource = std::make_shared<Texture>();
    std::shared_ptr<ResourceVersionFactory> factory = nullptr;

    uint32_t version = reader->ReadUInt32();
    switch (version) {
        case 0:
            factory = std::make_shared<TextureFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Texture with version {}", version);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void TextureFactoryV0::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
    std::shared_ptr<Texture> texture = std::static_pointer_cast<Texture>(resource);
    ResourceVersionFactory::ParseFileBinary(reader, texture);

    texture->Type = (TextureType)reader->ReadUInt32();
    texture->Width = reader->ReadUInt32();
    texture->Height = reader->ReadUInt32();

    uint32_t dataSize = reader->ReadUInt32();

    texture->ImageDataSize = dataSize;
    texture->ImageData = new uint8_t[dataSize];

    for (uint32_t i = 0; i < dataSize; i++) {
        texture->ImageData[i] = reader->ReadUByte();
    }
}
} // namespace Ship
