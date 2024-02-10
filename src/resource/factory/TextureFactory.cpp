#include "resource/factory/TextureFactory.h"
#include "resource/type/Texture.h"
#include "spdlog/spdlog.h"

namespace LUS {

std::shared_ptr<IResource> ResourceFactoryBinaryTextureV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto texture = std::make_shared<Texture>(file->InitData);

    texture->Type = (TextureType)file->Reader->ReadUInt32();
    texture->Width = file->Reader->ReadUInt32();
    texture->Height = file->Reader->ReadUInt32();
    texture->ImageDataSize = file->Reader->ReadUInt32();
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    file->Reader->Read((char*)texture->ImageData, texture->ImageDataSize);

    return texture;
}

std::shared_ptr<IResource> ResourceFactoryBinaryTextureV1::ReadResource(std::shared_ptr<File> file) {
if (!FileHasValidFormatAndReader(file)) {
    return nullptr;
}

    auto texture = std::make_shared<Texture>(file->InitData);

    texture->Type = (TextureType)file->Reader->ReadUInt32();
    texture->Width = file->Reader->ReadUInt32();
    texture->Height = file->Reader->ReadUInt32();
    texture->Flags = file->Reader->ReadUInt32();
    texture->HByteScale = file->Reader->ReadFloat();
    texture->VPixelScale = file->Reader->ReadFloat();
    texture->ImageDataSize = file->Reader->ReadUInt32();
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    file->Reader->Read((char*)texture->ImageData, texture->ImageDataSize);

    return texture;
}
} // namespace LUS
