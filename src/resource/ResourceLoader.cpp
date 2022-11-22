#include "ResourceLoader.h"
#include "ResourceFactory.h"
#include "ResourceMgr.h"
#include "Resource.h"
#include "OtrFile.h"
#include "binarytools/MemoryStream.h"
#include "binarytools/BinaryReader.h"
#include "factory/TextureFactory.h"
#include "factory/VertexFactory.h"
#include "factory/ArrayFactory.h"
#include "factory/BlobFactory.h"
#include "factory/DisplayListFactory.h"
#include "factory/MatrixFactory.h"

namespace Ship {
ResourceLoader::ResourceLoader(std::shared_ptr<Window> context) : mContext(context) {
    RegisterGlobalResourceFactories();
}

void ResourceLoader::RegisterGlobalResourceFactories() {
    RegisterResourceFactory(ResourceType::Texture, std::make_shared<TextureFactory>());
    RegisterResourceFactory(ResourceType::Vertex, std::make_shared<VertexFactory>());
    RegisterResourceFactory(ResourceType::DisplayList, std::make_shared<DisplayListFactory>());
    RegisterResourceFactory(ResourceType::Matrix, std::make_shared<MatrixFactory>());
    RegisterResourceFactory(ResourceType::Array, std::make_shared<ArrayFactory>());
    RegisterResourceFactory(ResourceType::Blob, std::make_shared<BlobFactory>());
}

bool ResourceLoader::RegisterResourceFactory(ResourceType resourceType, std::shared_ptr<ResourceFactory> factory) {
    if (mFactories.contains(resourceType)) {
        return false;
    }

    mFactories[resourceType] = factory;
    return true;
}

std::shared_ptr<Window> ResourceLoader::GetContext() {
    return mContext;
}

std::shared_ptr<Resource> ResourceLoader::LoadResource(std::shared_ptr<OtrFile> fileToLoad) {
    auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer.get(), fileToLoad->BufferSize);
    auto reader = std::make_shared<BinaryReader>(stream);

    // OTR HEADER BEGIN
    Endianness endianness = (Endianness)reader->ReadInt8();
    for (int i = 0; i < 3; i++) {
        reader->ReadInt8();
    }
    reader->SetEndianness(endianness);
    ResourceType resourceType = (ResourceType)reader->ReadUInt32(); // The type of the resource
    uint64_t id = reader->ReadUInt64();                             // Unique asset ID
    reader->ReadUInt32();                                           // Resource minor version number
    reader->ReadUInt64();                                           // ROM CRC
    reader->ReadUInt32();                                           // ROM Enum
    reader->Seek(64, SeekOffsetType::Start);                        // Reserved for future file format versions...
    // OTR HEADER END

    std::shared_ptr<Resource> result = nullptr;

    auto factory = mFactories[resourceType];

    if (factory != nullptr) {
        result = factory->ReadResource(reader);
    }

    if (result != nullptr) {
        result->File = fileToLoad;
        result->Id = id;
        result->Type = resourceType;
    } else {
        if (fileToLoad != nullptr) {
            SPDLOG_ERROR("Failed to load resource of type {} \"{}\"", (uint32_t)resourceType, fileToLoad->Path);
        } else {
            SPDLOG_ERROR("Failed to load resource because the file did not load.");
        }
    }

    return result;
}
} // namespace Ship
