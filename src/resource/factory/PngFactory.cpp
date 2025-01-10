#include "resource/factory/PngFactory.h"
#include "resource/type/Texture.h"
#include "spdlog/spdlog.h"

#include <stb_image.h>

namespace Fast {

std::shared_ptr<Ship::IResource>
ResourceFactoryImageTexture::ReadResource(std::shared_ptr<Ship::File> file,
                                          std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto texture = std::make_shared<Texture>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    auto callback = stbi_io_callbacks{ [](void* user, char* data, int size) {
                                          auto reader = static_cast<Ship::BinaryReader*>(user);
                                          reader->Read(data, size);
                                          return size;
                                      },
                                       [](void* user, int size) {
                                           auto reader = static_cast<Ship::BinaryReader*>(user);
                                           reader->Seek(size, Ship::SeekOffsetType::Current);
                                       },
                                       [](void* user) { return 0; } };
    int height, width = 0;

    texture->ImageData = stbi_load_from_callbacks(&callback, reader.get(), &width, &height, nullptr, 4);

    texture->Width = width;
    texture->Height = height;
    texture->Type = TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;

    return texture;
}

} // namespace Fast