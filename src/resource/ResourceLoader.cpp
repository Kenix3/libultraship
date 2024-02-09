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
    // include strings so we register types for mResourceTypes
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Texture), std::make_shared<TextureFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Vertex), std::make_shared<VertexFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::DisplayList), std::make_shared<DisplayListFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Matrix), std::make_shared<MatrixFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Array), std::make_shared<ArrayFactory>());
    RegisterResourceFactory(static_cast<uint32_t>(ResourceType::Blob), std::make_shared<BlobFactory>());
}

bool ResourceLoader::RegisterResourceFactory(std::shared_ptr<ResourceFactory> factory, uint32_t format, std::string typeName, uint32_t type, uint32_t version) {
    if (mResourceTypes.contains(typeName)) {
        return false;
    }
    mResourceTypes[typeName] = type;

    ResourceFactoryKey key = {resourceFormat: format,  resourceType: type, resourceVersion: version};
    if (mFactories.contains(key)) {
        return false;
    }
    mFactories[key] = factory;

    return true;
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

    // call method to get factory based on factorykey (generate using params)
    // make a method that takes in a string instead of an int for the type too
    // make those protected

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
        SPDLOG_ERROR("Failed to load resource of type {} \"{}\"", fileToLoad->InitData->Type,
                     fileToLoad->InitData->Path);
    }

    return result;
}
} // namespace LUS
