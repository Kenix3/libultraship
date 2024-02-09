#include "resource/factory/BlobFactory.h"
#include "resource/type/Blob.h"
#include "resource/readerbox/BinaryReaderBox.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryBlobV0::ReadResource(std::shared_ptr<ResourceInitData> initData,
                                                        std::shared_ptr<ReaderBox> readerBox) {
    auto binaryReaderBox = std::dynamic_pointer_cast<BinaryReaderBox>(readerBox);
    if (binaryReaderBox == nullptr) {
        SPDLOG_ERROR("ReaderBox must be a BinaryReaderBox.");
        return nullptr;
    }

    auto reader = binaryReaderBox->GetReader();
    if (reader == nullptr) {
        SPDLOG_ERROR("null reader in box.");
        return nullptr;
    }

    auto blob = std::make_shared<Blob>(initData);

    uint32_t dataSize = reader->ReadUInt32();

    blob->Data.reserve(dataSize);

    for (uint32_t i = 0; i < dataSize; i++) {
        blob->Data.push_back(reader->ReadUByte());
    }

    return blob;
}
} // namespace LUS
