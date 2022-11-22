#include "core/bridge/resourcebridge.h"
#include "core/Window.h"
#include "resource/ResourceMgr.h"
#include <string>
#include <algorithm>
#include <StrHash64.h>

#include "resource/type/Texture.h"

std::shared_ptr<Ship::Resource> LoadResource(const char* name) {
    return Ship::Window::GetInstance()->GetResourceManager()->LoadResource(name);
}

std::shared_ptr<Ship::Resource> LoadResource(uint64_t crc) {
    return LoadResource(GetResourceNameByCrc(crc));
}

extern "C" {

uint64_t GetResourceCrcByName(const char* name) {
    return CRC64(name);
}

const char* GetResourceNameByCrc(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);
    return hashStr != nullptr ? hashStr->c_str() : nullptr;
}

size_t GetResourceSizeByName(const char* name) {
    auto resource = LoadResource(name);

    if (resource == nullptr) {
        return 0;
    }

    return resource->GetPointerSize();
}

size_t GetResourceSizeByCrc(uint64_t crc) {
    return GetResourceSizeByName(GetResourceNameByCrc(crc));
}

void* GetResourceDataByName(const char* name) {
    auto resource = LoadResource(name);

    if (resource == nullptr) {
        return nullptr;
    }

    return resource->GetPointer();
}

void* GetResourceDataByCrc(uint64_t crc) {
    return GetResourceDataByName(GetResourceNameByCrc(crc));
}

uint16_t GetResourceTexWidthByName(const char* name) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexWidthByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexHeightByName(const char* name) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexHeightByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t GetResourceTexSizeByName(const char* name) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t GetResourceTexSizeByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

void GetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount) {
    auto list = Ship::Window::GetInstance()->GetResourceManager()->GetGameVersions();
    memcpy(versions, list.data(), std::min(versionsSize, list.size() * sizeof(uint32_t)));
    *versionsCount = list.size();
}

uint32_t HasGameVersion(uint32_t hash) {
    auto list = Ship::Window::GetInstance()->GetResourceManager()->GetGameVersions();
    return std::find(list.begin(), list.end(), hash) != list.end();
}

void LoadResourceDirectory(const char* name) {
    Ship::Window::GetInstance()->GetResourceManager()->CacheDirectory(name);
}

void DirtyResourceDirectory(const char* name) {
    Ship::Window::GetInstance()->GetResourceManager()->DirtyDirectory(name);
}

void DirtyResourceByName(const char* name) {
    auto resource = LoadResource(name);

    if (resource != nullptr) {
        resource->IsDirty = true;
    }
}

void DirtyResourceByCrc(uint64_t crc) {
    auto resource = LoadResource(crc);

    if (resource != nullptr) {
        resource->IsDirty = true;
    }
}

size_t UnloadResourceByName(const char* name) {
    return Ship::Window::GetInstance()->GetResourceManager()->UnloadResource(name);
}
size_t UnloadResourceByCrc(uint64_t crc) {
    return UnloadResourceByName(GetResourceNameByCrc(crc));
}

void ClearResourceCache(void) {
    Ship::Window::GetInstance()->GetResourceManager()->InvalidateResourceCache();
}

void RegisterResourcePatchByName(const char* name, size_t index, uintptr_t origData) {
    const auto res = LoadResource(name);

    if (res != nullptr) {
        const auto hash = GetResourceCrcByName(name);
        Ship::ResourceAddressPatch patch;
        patch.ResourceCrc = hash;
        patch.InstructionIndex = index;
        patch.OriginalData = origData;

        res->Patches.push_back(patch);
    }
}

void RegisterResourcePatchByCrc(uint64_t crc, size_t index, uintptr_t origData) {
    const auto res = LoadResource(crc);

    if (res != nullptr) {
        Ship::ResourceAddressPatch patch;
        patch.ResourceCrc = crc;
        patch.InstructionIndex = index;
        patch.OriginalData = origData;

        res->Patches.push_back(patch);
    }
}

void WriteTextureDataInt16ByName(const char* name, size_t index, int16_t valueToWrite) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(name));

    if (res != nullptr) {
        if ((index * sizeof(int16_t)) < res->ImageDataSize) {
            ((int16_t*)res->ImageData)[index] = valueToWrite;
        }
    }
}

void WriteTextureDataInt16ByCrc(uint64_t crc, size_t index, int16_t valueToWrite) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        if ((index * sizeof(int16_t)) < res->ImageDataSize) {
            ((int16_t*)res->ImageData)[index] = valueToWrite;
        }
    }
}
}
