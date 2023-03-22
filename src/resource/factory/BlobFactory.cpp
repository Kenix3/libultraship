#include "resource/factory/BlobFactory.h"
#include "resource/type/Blob.h"
#include "spdlog/spdlog.h"

namespace Ship {
std::shared_ptr<Resource> BlobFactory::ReadResource(std::shared_ptr<ResourceMgr> resourceMgr,
                                                    std::shared_ptr<ResourceInitData> initData,
                                                    std::shared_ptr<BinaryReader> reader) {
    auto resource = std::make_shared<Blob>(resourceMgr, initData);
    std::shared_ptr<ResourceVersionFactory> factory = nullptr;

    switch (resource->InitData->ResourceVersion) {
        case 0:
            factory = std::make_shared<BlobFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Blob with version {}", resource->InitData->ResourceVersion);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void BlobFactoryV0::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
    std::shared_ptr<Blob> blob = std::static_pointer_cast<Blob>(resource);
    ResourceVersionFactory::ParseFileBinary(reader, blob);

    uint32_t dataSize = reader->ReadUInt32();

    blob->Data.reserve(dataSize);

    for (uint32_t i = 0; i < dataSize; i++) {
        blob->Data.push_back(reader->ReadUByte());
    }
}
} // namespace Ship
