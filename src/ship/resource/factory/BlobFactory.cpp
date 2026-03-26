#include "ship/resource/factory/BlobFactory.h"
#include "ship/resource/type/Blob.h"
#include "spdlog/spdlog.h"

namespace Ship {
std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryBlobV0::ReadResource(std::shared_ptr<Ship::File> file,
                                          std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto blob = std::make_shared<Blob>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    uint32_t dataSize = reader->ReadUInt32();

    // Small zero-pad for N64 code that overreads by a few bytes
    // (e.g. compressed MIDI parser). Large audio DMA overreads are handled by
    // AudioDma_Clamp in osPiStartDma instead.
    constexpr uint32_t kBlobPadding = 16;
    blob->Data.reserve(dataSize + kBlobPadding);

    for (uint32_t i = 0; i < dataSize; i++) {
        blob->Data.push_back(reader->ReadUByte());
    }

    blob->Data.resize(dataSize + kBlobPadding, 0);

    return blob;
}
} // namespace Ship
