#include "Blob.h"

namespace Ship {
std::shared_ptr<Resource> BlobFactory::ReadResource(std::shared_ptr<BinaryReader> reader) {
    auto resource = std::make_shared<Blob>();
    std::shared_ptr<ResourceVersionFactory> factory = nullptr;

    uint32_t version = reader->ReadUInt32();
    switch (version) {
        case 0:
            factory = std::make_shared<BlobFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Vertex with version {}", version);
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

void* Blob::GetPointer() {
    return Data.data();
}

size_t Blob::GetPointerSize() {
    return Data.size() * sizeof(uint8_t);
}
} // namespace Ship
