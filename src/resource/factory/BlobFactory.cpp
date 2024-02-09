#include "resource/factory/BlobFactory.h"
#include "resource/type/Blob.h"
#include "resource/readerbox/BinaryReaderBox.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryBlobV0::ReadResource(std::shared_ptr<File> file) {
    if (file->InitData->Format != RESOURCE_FORMAT_BINARY) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return nullptr;
    }

    if (file->Reader == nullptr) {
        SPDLOG_ERROR("Failed to load resource: File has Reader ({} - {})", file->InitData->Type,
                        file->InitData->Path);
        return nullptr;
    }

    auto blob = std::make_shared<Blob>(file->InitData);

    uint32_t dataSize = file->Reader->ReadUInt32();

    blob->Data.reserve(dataSize);

    for (uint32_t i = 0; i < dataSize; i++) {
        blob->Data.push_back(file->Reader->ReadUByte());
    }

    return blob;
}
} // namespace LUS
