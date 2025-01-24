#include "resource/factory/TextureFactory.h"
#include "resource/type/Texture.h"
#include "spdlog/spdlog.h"
#include <Context.h>
#include <File.h>
#include <stb_image.h>

namespace Fast {

std::shared_ptr<Ship::IResource> loadPngTexture(std::shared_ptr<Ship::File> filePng) {
    auto texture = std::make_shared<Texture>(filePng->InitData);

    int height, width = 0;
    texture->ImageData = stbi_load_from_memory((const stbi_uc*)filePng->Buffer.get()->data(),
                                               filePng->Buffer.get()->size(), &width, &height, nullptr, 4);
    texture->Width = width;
    texture->Height = height;
    texture->Type = TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;
    return texture;
}

std::shared_ptr<Ship::IResource> ResourceFactoryBinaryTextureV0::ReadResource(std::shared_ptr<Ship::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }
    auto initData = std::make_shared<Ship::ResourceInitData>();
    initData->Format = RESOURCE_FORMAT_BINARY;
    initData->Type = static_cast<uint32_t>(Ship::ResourceType::Blob);
    initData->ResourceVersion = 0;

    auto filePng = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->LoadFile(
        file->InitData->Path + ".png", initData);

    if (filePng != nullptr) {
        return loadPngTexture(filePng);
    }

    auto texture = std::make_shared<Texture>(file->InitData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    texture->Type = (TextureType)reader->ReadUInt32();
    texture->Width = reader->ReadUInt32();
    texture->Height = reader->ReadUInt32();
    texture->ImageDataSize = reader->ReadUInt32();
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    reader->Read((char*)texture->ImageData, texture->ImageDataSize);

    return texture;
}

std::shared_ptr<Ship::IResource> ResourceFactoryBinaryTextureV1::ReadResource(std::shared_ptr<Ship::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto texture = std::make_shared<Texture>(file->InitData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

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
} // namespace Fast
