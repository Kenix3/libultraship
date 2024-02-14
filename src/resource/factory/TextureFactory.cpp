#include "resource/factory/TextureFactory.h"
#include "resource/type/Texture.h"
#include "spdlog/spdlog.h"

namespace LUS {

std::shared_ptr<IResource> ResourceFactoryBinaryTextureV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto texture = std::make_shared<Texture>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    texture->Type = (TextureType)reader->ReadUInt32();
    texture->Width = reader->ReadUInt32();
    texture->Height = reader->ReadUInt32();
    texture->ImageDataSize = reader->ReadUInt32();
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    reader->Read((char*)texture->ImageData, texture->ImageDataSize);

    return texture;
}

std::shared_ptr<IResource> ResourceFactoryBinaryTextureV1::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto texture = std::make_shared<Texture>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    texture->Type = (TextureType)reader->ReadUInt32();
    texture->Width = reader->ReadUInt32();
    texture->Height = reader->ReadUInt32();
    texture->Flags = reader->ReadUInt32();
    texture->HByteScale = reader->ReadFloat();
    texture->VPixelScale = reader->ReadFloat();
    texture->ImageDataSize = reader->ReadUInt32();
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    reader->Read((char*)texture->ImageData, texture->ImageDataSize);

    return texture;
}
} // namespace LUS
