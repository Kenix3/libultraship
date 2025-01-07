#include "resource/factory/PngFactory.h"
#include "resource/type/Texture.h"
#include "spdlog/spdlog.h"

#include <png.h>

namespace Fast {

extern "C" {
void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    SPDLOG_ERROR("libpng error: {}", error_msg);
}

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    SPDLOG_WARN("libpng warning: {}", warning_msg);
}
}

std::shared_ptr<Ship::IResource> ResourceFactoryPngTexture::ReadResource(std::shared_ptr<Ship::File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto texture = std::make_shared<Texture>(file->InitData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    png_structp png_ptr =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) nullptr, user_error_fn, user_warning_fn);
    if (!png_ptr) {
        SPDLOG_ERROR("Failed to create PNG read struct for file: {}", file->InitData->Path);
        return nullptr;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp) nullptr, (png_infopp) nullptr);
        SPDLOG_ERROR("Failed to create PNG read struct for file: {}", file->InitData->Path);
        return nullptr;
    }
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) nullptr);
        SPDLOG_ERROR("Failed to create PNG read struct for file: {}", file->InitData->Path);
        return nullptr;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        SPDLOG_ERROR("Failed to create PNG read struct for file: {}", file->InitData->Path);
        return nullptr;
    }

    png_set_read_fn(png_ptr, reader.get(), [](png_structp png_ptr, png_bytep data, png_size_t length) {
        auto reader = (Ship::BinaryReader*)png_get_io_ptr(png_ptr);
        reader->Read((char*)data, length);
    });

    png_read_info(png_ptr, info_ptr);
}

} // namespace Fast