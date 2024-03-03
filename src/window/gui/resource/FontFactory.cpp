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

    uint32_t dataSize = file->Buffer->size();

    font->Data.reserve(dataSize);

    for (uint32_t i = 0; i < dataSize; i++) {
        font->Data.push_back(reader->ReadChar());
    }

    return font;
}
} // namespace LUS
