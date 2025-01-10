#include "ResourceFactoryBinary.h"
#include <variant>
#include "spdlog/spdlog.h"

namespace Ship {
bool ResourceFactoryBinary::FileHasValidFormatAndReader(std::shared_ptr<File> file,
                                                        std::shared_ptr<Ship::ResourceInitData> initData) {
    if (initData->Format != RESOURCE_FORMAT_BINARY) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return false;
    }

    if (!std::holds_alternative<std::shared_ptr<BinaryReader>>(file->Reader)) {
        SPDLOG_ERROR("Failed to load resource: File has Reader ({} - {})", initData->Type, initData->Path);
        return false;
    }

    return true;
};
} // namespace Ship
