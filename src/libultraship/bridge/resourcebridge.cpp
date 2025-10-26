#include "libultraship/bridge/resourcebridge.h"
#include "ship/Context.h"
#include <string>
#include <algorithm>
#include "ship/utils/StrHash64.h"
#include "ship/window/Window.h"

std::shared_ptr<Ship::IResource> ResourceLoad(const char* name) {
    return Ship::Context::GetInstance()->GetResourceManager()->LoadResource(name);
}

std::shared_ptr<Ship::IResource> ResourceLoad(uint64_t crc) {
    return Ship::Context::GetInstance()->GetResourceManager()->LoadResource(crc);
}

extern "C" {

uint64_t ResourceGetCrcByName(const char* name) {
    return CRC64(name);
}

const char* ResourceGetNameByCrc(uint64_t crc) {
    return Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToCString(crc);
}

size_t ResourceGetSizeByName(const char* name) {
    return Ship::Context::GetInstance()->GetResourceManager()->GetResourceSize(name);
}

size_t ResourceGetSizeByCrc(uint64_t crc) {
    return Ship::Context::GetInstance()->GetResourceManager()->GetResourceSize(crc);
}

uint8_t ResourceGetIsCustomByName(const char* name) {
    return Ship::Context::GetInstance()->GetResourceManager()->GetResourceIsCustom(name);
}

uint8_t ResourceGetIsCustomByCrc(uint64_t crc) {
    return Ship::Context::GetInstance()->GetResourceManager()->GetResourceIsCustom(crc);
}

void* ResourceGetDataByName(const char* name) {
    return Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(name);
}

void* ResourceGetDataByCrc(uint64_t crc) {
    return Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(crc);
}

uint16_t ResourceGetTexWidthByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceGetTexWidthByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceGetTexHeightByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceGetTexHeightByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t ResourceGetTexSizeByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t ResourceGetTexSizeByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

void ResourceGetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount) {
    auto list = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions();
    memcpy(versions, list.data(), std::min(versionsSize, list.size() * sizeof(uint32_t)));
    *versionsCount = list.size();
}

void ResourceLoadDirectoryAsync(const char* name) {
    Ship::Context::GetInstance()->GetResourceManager()->LoadResourcesAsync(name);
}

uint32_t ResourceHasGameVersion(uint32_t hash) {
    auto list = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions();
    return std::find(list.begin(), list.end(), hash) != list.end();
}

void ResourceLoadDirectory(const char* name) {
    Ship::Context::GetInstance()->GetResourceManager()->LoadResources(name);
}

void ResourceDirtyDirectory(const char* name) {
    Ship::Context::GetInstance()->GetResourceManager()->DirtyResources(name);
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
    Ship::Context::GetInstance()->GetResourceManager()->UnloadResource(name);
}

void ResourceUnloadByCrc(uint64_t crc) {
    ResourceUnloadByName(ResourceGetNameByCrc(crc));
}

void ResourceUnloadDirectory(const char* name) {
    Ship::Context::GetInstance()->GetResourceManager()->UnloadResources(name);
}

uint32_t IsResourceManagerLoaded() {
    return Ship::Context::GetInstance()->GetResourceManager()->IsLoaded();
}
}
