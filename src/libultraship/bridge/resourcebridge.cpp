#include "libultraship/bridge/resourcebridge.h"
#include "ship/resource/ResourceManager.h"
#include <string>
#include <algorithm>
#include <cstring>
#include "ship/utils/StrHash64.h"
#include "ship/window/Window.h"

static std::shared_ptr<Ship::ResourceManager> sResourceManager;

void ResourceSetResourceManager(std::shared_ptr<Ship::ResourceManager> resourceManager) {
    sResourceManager = std::move(resourceManager);
}

std::shared_ptr<Ship::ResourceManager> ResourceGetResourceManager() {
    return sResourceManager;
}

std::shared_ptr<Ship::IResource> ResourceLoad(const char* name) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->LoadResource(name) : nullptr;
}

std::shared_ptr<Ship::IResource> ResourceLoad(uint64_t crc) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->LoadResource(crc) : nullptr;
}

extern "C" {

uint64_t ResourceGetCrcByName(const char* name) {
    return CRC64(name);
}

const char* ResourceGetNameByCrc(uint64_t crc) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->GetArchiveManager()->HashToCString(crc) : nullptr;
}

size_t ResourceGetSizeByName(const char* name) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->GetResourceSize(name) : 0;
}

size_t ResourceGetSizeByCrc(uint64_t crc) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->GetResourceSize(crc) : 0;
}

uint8_t ResourceGetIsCustomByName(const char* name) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->GetResourceIsCustom(name) : 0;
}

uint8_t ResourceGetIsCustomByCrc(uint64_t crc) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->GetResourceIsCustom(crc) : 0;
}

void* ResourceGetDataByName(const char* name) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->GetResourceRawPointer(name) : nullptr;
}

void* ResourceGetDataByCrc(uint64_t crc) {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->GetResourceRawPointer(crc) : nullptr;
}

uint16_t ResourceGetTexWidthByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return static_cast<uint16_t>(-1);
}

uint16_t ResourceGetTexWidthByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return static_cast<uint16_t>(-1);
}

uint16_t ResourceGetTexHeightByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return static_cast<uint16_t>(-1);
}

uint16_t ResourceGetTexHeightByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return static_cast<uint16_t>(-1);
}

size_t ResourceGetTexSizeByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return static_cast<size_t>(-1);
}

size_t ResourceGetTexSizeByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return static_cast<size_t>(-1);
}

void ResourceGetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount) {
    auto resourceManager = ResourceGetResourceManager();
    if (!resourceManager) {
        *versionsCount = 0;
        return;
    }

    auto list = resourceManager->GetArchiveManager()->GetGameVersions();
    memcpy(versions, list.data(), std::min(versionsSize, list.size() * sizeof(uint32_t)));
    *versionsCount = list.size();
}

void ResourceLoadDirectoryAsync(const char* name) {
    if (auto resourceManager = ResourceGetResourceManager()) {
        resourceManager->LoadResourcesAsync(name);
    }
}

uint32_t ResourceHasGameVersion(uint32_t hash) {
    auto resourceManager = ResourceGetResourceManager();
    if (!resourceManager) {
        return 0;
    }

    auto list = resourceManager->GetArchiveManager()->GetGameVersions();
    return std::find(list.begin(), list.end(), hash) != list.end();
}

void ResourceLoadDirectory(const char* name) {
    if (auto resourceManager = ResourceGetResourceManager()) {
        resourceManager->LoadResources(name);
    }
}

void ResourceDirtyDirectory(const char* name) {
    if (auto resourceManager = ResourceGetResourceManager()) {
        resourceManager->DirtyResources(name);
    }
}

void ResourceDirtyByName(const char* name) {
    auto resource = ResourceLoad(name);

    if (resource != nullptr) {
        resource->Dirty();
    }
}

void ResourceDirtyByCrc(uint64_t crc) {
    auto resource = ResourceLoad(crc);

    if (resource != nullptr) {
        resource->Dirty();
    }
}

void ResourceUnloadByName(const char* name) {
    if (auto resourceManager = ResourceGetResourceManager()) {
        resourceManager->UnloadResource(name);
    }
}

void ResourceUnloadByCrc(uint64_t crc) {
    auto name = ResourceGetNameByCrc(crc);
    if (name) {
        ResourceUnloadByName(name);
    }
}

void ResourceUnloadDirectory(const char* name) {
    if (auto resourceManager = ResourceGetResourceManager()) {
        resourceManager->UnloadResources(name);
    }
}

uint32_t IsResourceManagerLoaded() {
    auto resourceManager = ResourceGetResourceManager();
    return resourceManager ? resourceManager->IsInitialized() : 0;
}
}
