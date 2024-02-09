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
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY, "Texture", static_cast<uint32_t>(ResourceType::Texture), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY, "Texture", static_cast<uint32_t>(ResourceType::Texture), 1);
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY, "Vertex", static_cast<uint32_t>(ResourceType::Vertex), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryXMLVertexV0>(), RESOURCE_FORMAT_XML, "Vertex", static_cast<uint32_t>(ResourceType::Vertex), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryDisplayListV0>(), RESOURCE_FORMAT_BINARY, "DisplayList", static_cast<uint32_t>(ResourceType::DisplayList), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryXMLDisplayListV0>(), RESOURCE_FORMAT_XML, "DisplayList", static_cast<uint32_t>(ResourceType::DisplayList), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryMatrixV0>(), RESOURCE_FORMAT_BINARY, "Matrix", static_cast<uint32_t>(ResourceType::Matrix), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryArrayV0>(), RESOURCE_FORMAT_BINARY, "Array", static_cast<uint32_t>(ResourceType::Array), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryBlobV0>(), RESOURCE_FORMAT_BINARY, "Blob", static_cast<uint32_t>(ResourceType::Blob), 0);
}

bool ResourceLoader::RegisterResourceFactory(std::shared_ptr<ResourceFactory> factory, uint32_t format, std::string typeName, uint32_t type, uint32_t version) {
    if (mResourceTypes.contains(typeName)) {
        if(mResourceTypes[typeName] != type) {
            SPDLOG_ERROR("Failed to register resource factory: conflicting types for name {}", typeName);
            return false;
        }
    } else {
        mResourceTypes[typeName] = type;
    }

    ResourceFactoryKey key = {resourceFormat: format,  resourceType: type, resourceVersion: version};
    if (mFactories.contains(key)) {
        SPDLOG_ERROR("Failed to register resource factory: factory with key {}{}{} already exists", format, type, version);
        return false;
    }
    mFactories[key] = factory;

    return true;
}

std::shared_ptr<ResourceFactory> ResourceLoader::GetFactory(uint32_t format, uint32_t type, uint32_t version) {
    ResourceFactoryKey key = {resourceFormat: format,  resourceType: type, resourceVersion: version};
    if (!mFactories.contains(key)) {
        SPDLOG_ERROR("Could not find resource factory with key {}{}{}", format, type, version);
        return nullptr;
    }

    return mFactories[key];
}

std::shared_ptr<ResourceFactory> ResourceLoader::GetFactory(uint32_t format, std::string typeName, uint32_t version) {
    if (!mResourceTypes.contains(typeName)) {
        SPDLOG_ERROR("Could not find resource type for name {}", typeName);
        return nullptr;
    }

    return GetFactory(format, mResourceTypes[typeName], version);
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



    auto factory = GetFactory(fileToLoad->InitData->IsXml) mFactories[fileToLoad->InitData->Type];
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
