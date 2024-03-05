#include "window/gui/resource/FontFactory.h"
#include "window/gui/resource/Font.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryFontV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto font = std::make_shared<Font>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    font->DataSize = file->Buffer->size();

    font->Data = new char[font->DataSize];
    reader->Read(font->Data, font->DataSize);

    return font;
}
} // namespace LUS
