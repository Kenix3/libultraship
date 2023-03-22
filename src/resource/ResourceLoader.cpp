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
    RegisterResourceFactory(ResourceType::Texture, "Texture", std::make_shared<TextureFactory>());
    RegisterResourceFactory(ResourceType::Vertex, "Vertex", std::make_shared<VertexFactory>());
    RegisterResourceFactory(ResourceType::DisplayList, "DisplayList", std::make_shared<DisplayListFactory>());
    RegisterResourceFactory(ResourceType::Matrix, "Matrix", std::make_shared<MatrixFactory>());
    RegisterResourceFactory(ResourceType::Array, "Array", std::make_shared<ArrayFactory>());
    RegisterResourceFactory(ResourceType::Blob, "Blob", std::make_shared<BlobFactory>());
}

bool ResourceLoader::RegisterResourceFactory(ResourceType resourceType, std::string resourceTypeXML,
                                             std::shared_ptr<ResourceFactory> factory) {
    if (mFactories.contains(resourceType)) {
        return false;
    }

    mFactories[resourceType] = factory;
    mFactoriesStr[resourceTypeXML] = factory;
    mFactoriesTypes[resourceTypeXML] = resourceType;
    return true;
}

std::shared_ptr<Window> ResourceLoader::GetContext() {
    return mContext;
}

std::shared_ptr<Resource> ResourceLoader::LoadResource(std::shared_ptr<OtrFile> fileToLoad) {
    std::shared_ptr<Resource> result = nullptr;

    if (fileToLoad != nullptr) {
        auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer.data(), fileToLoad->Buffer.size());
        auto reader = std::make_shared<BinaryReader>(stream);

        // Determine if file is binary or XML...
        uint8_t firstByte = reader->ReadInt8();
        ResourceType resourceType;
        uint32_t resourceVersion = -1;
        uint64_t id = 0xDEADBEEFDEADBEEF;
        Endianness endianness = Endianness::Native;

        // If first byte is '<' then we are loading XML, else we are loading OTR binary.
        if (firstByte == '<') {
            // XML
            reader->Seek(-1, SeekOffsetType::Current);

            std::string xmlStr = reader->ReadCString();

            tinyxml2::XMLDocument doc;
            tinyxml2::XMLError eResult = doc.Parse(xmlStr.data());

            // OTRTODO: Error checking

            auto root = doc.FirstChildElement();

            std::string nodeName = root->Name();
            resourceVersion = root->IntAttribute("Version");

            auto factory = mFactoriesStr[nodeName];

            if (factory != nullptr) {
                resourceType = mFactoriesTypes[nodeName];

                auto resourceInitData = std::make_shared<ResourceInitData>();
                resourceInitData->Id = id;
                resourceInitData->Type = resourceType;
                resourceInitData->Path = fileToLoad->Path;
                resourceInitData->ByteOrder = endianness;
                resourceInitData->ResourceVersion = resourceVersion;
                result = factory->ReadResourceXML(GetContext()->GetResourceManager(), resourceInitData, root);
            }
        } else {
            // OTR HEADER BEGIN
            endianness = (Endianness)firstByte;
            for (int i = 0; i < 3; i++) {
                reader->ReadInt8();
            }
            reader->SetEndianness(endianness);
            resourceType = (ResourceType)reader->ReadUInt32(); // The type of the resource
            resourceVersion = reader->ReadUInt32();            // Resource version
            id = reader->ReadUInt64();                         // Unique asset ID
            reader->ReadUInt32();                              // ????
            reader->ReadUInt64();                              // ROM CRC
            reader->ReadUInt32();                              // ROM Enum
            reader->Seek(64, SeekOffsetType::Start);           // Reserved for future file format versions...
            // OTR HEADER END

            auto factory = mFactories[resourceType];

            if (factory != nullptr) {
                auto resourceInitData = std::make_shared<ResourceInitData>();
                resourceInitData->Id = id;
                resourceInitData->Type = resourceType;
                resourceInitData->Path = fileToLoad->Path;
                resourceInitData->ByteOrder = endianness;
                resourceInitData->ResourceVersion = resourceVersion;
                result = factory->ReadResource(GetContext()->GetResourceManager(), resourceInitData, reader);
            }
        }

        if (result == nullptr) {
            if (fileToLoad != nullptr) {
                SPDLOG_ERROR("Failed to load resource of type {} \"{}\"", (uint32_t)resourceType, fileToLoad->Path);
            } else {
                SPDLOG_ERROR("Failed to load resource because the file did not load.");
            }
        }
    }

    return result;
}
} // namespace Ship
