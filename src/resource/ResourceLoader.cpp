#include "ResourceLoader.h"
#include "ResourceFactory.h"
#include "ResourceManager.h"
#include "Resource.h"
#include "File.h"
#include "Context.h"
#include "utils/binarytools/MemoryStream.h"
#include "utils/binarytools/BinaryReader.h"
#include "factory/TextureFactory.h"
#include "factory/VertexFactory.h"
#include "factory/ArrayFactory.h"
#include "factory/BlobFactory.h"
#include "factory/DisplayListFactory.h"
#include "factory/MatrixFactory.h"

namespace LUS {
ResourceLoader::ResourceLoader() {
    RegisterGlobalResourceFactories();
}

ResourceLoader::~ResourceLoader() {
    SPDLOG_TRACE("destruct resource loader");
}

void ResourceLoader::RegisterGlobalResourceFactories() {
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Texture), std::make_shared<TextureFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Vertex), std::make_shared<VertexFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::DisplayList), std::make_shared<DisplayListFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Matrix), std::make_shared<MatrixFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Array), std::make_shared<ArrayFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Blob), std::make_shared<BlobFactory>());
}

bool ResourceLoader::RegisterResourceFactory(uint32_t resourceType, std::shared_ptr<ResourceFactory> factory) {
    if (mFactories.contains(resourceType)) {
        return false;
    }

    mFactories[resourceType] = factory;
    return true;
}

uint32_t ResourceLoader::GetVersionFromString(const std::string& version) {
    return mFactoriesTypes[version];
}

std::shared_ptr<IResource> ResourceLoader::LoadResource(std::shared_ptr<File> fileToLoad) {
    std::shared_ptr<IResource> result = nullptr;

    if (fileToLoad == nullptr) {
        SPDLOG_ERROR("Failed to load resource: File not loaded");
        return result;
    }

    if (fileToLoad->InitData == nullptr) {
        SPDLOG_ERROR("Failed to load resource: ResourceInitData not loaded");
        return result;
    }

    auto factory = mFactories[fileToLoad->InitData->Type];
    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load resource: Factory does not exist ({} - {})", fileToLoad->InitData->Type,
                     fileToLoad->InitData->Path);
    }

    if (fileToLoad->InitData->IsXml) {
        if (fileToLoad->XmlDocument == nullptr) {
            SPDLOG_ERROR("Failed to load resource: File has no XML document ({} - {})", fileToLoad->InitData->Type,
                         fileToLoad->InitData->Path);
            return result;
        }

        result = factory->ReadResourceXML(fileToLoad->InitData, fileToLoad->XmlDocument->FirstChildElement());
    } else {
        if (fileToLoad->Reader == nullptr) {
            SPDLOG_ERROR("Failed to load resource: File has Reader ({} - {})", fileToLoad->InitData->Type,
                         fileToLoad->InitData->Path);
            return result;
        }

        result = factory->ReadResource(fileToLoad->InitData, fileToLoad->Reader);
    }

    if (result == nullptr) {
        SPDLOG_ERROR("Failed to load resource of type {} \"{}\"", fileToLoad->InitData->Type, fileToLoad->InitData->Path);
    }

    return result;
}
} // namespace LUS
