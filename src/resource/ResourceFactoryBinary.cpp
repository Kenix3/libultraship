#include "ResourceFactoryBinary.h"
#include "spdlog/spdlog.h"

namespace LUS {
bool ResourceFactoryBinary::FileHasValidFormatAndReader(std::shared_ptr<File> file) {
    if (file->InitData->Format != RESOURCE_FORMAT_BINARY) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return false;
    }

    if (file->Reader == nullptr) {
        SPDLOG_ERROR("Failed to load resource: File has Reader ({} - {})", file->InitData->Type, file->InitData->Path);
        return false;
    }

    return true;
};
} // namespace LUS