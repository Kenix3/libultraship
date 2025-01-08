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
    texture->Width = png_get_image_width(png_ptr, info_ptr);
    texture->Height = png_get_image_height(png_ptr, info_ptr);
    texture->Type = TextureType::RGBA32bpp;
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->ImageData = new uint8_t[texture->ImageDataSize];

    for (int y = 0; y < texture->Height; y++) {
        png_bytep row = (png_bytep)texture->ImageData + (y * texture->Width * 4);
        png_read_row(png_ptr, row, nullptr);
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    texture->Flags = TEX_FLAG_LOAD_AS_RAW;

    return texture;
}

} // namespace Fast