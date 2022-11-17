#include "resourcebridge.h"
#include "core/Window.h"
#include "resource/ResourceMgr.h"
#include <string>
#include <algorithm>
#include <StrHash64.h>

#include "resource/types/Texture.h"

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

    return resource->File->BufferSize;
}

size_t GetResourceSizeByCrc(uint64_t crc) {
    auto resource = LoadResource(crc);

    if (resource == nullptr) {
        return 0;
    }
    return resource->File->BufferSize;
}

void* GetResourceDataByName(const char* name) {
    auto resource = LoadResource(name);

    if (resource == nullptr) {
        return nullptr;
    }

    auto buffer = resource->File->Buffer;
    return buffer.get();
}

void* GetResourceDataByCrc(uint64_t crc) {
    return GetResourceDataByName(GetResourceNameByCrc(crc));
}

uint16_t GetResourceTexWidthByName(const char* name) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexWidthByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexHeightByName(const char* name) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexHeightByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t GetResourceTexSizeByName(const char* name) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->imageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t GetResourceTexSizeByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->imageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint32_t GetGameVersion(void) {
    return Ship::Window::GetInstance()->GetResourceManager()->GetGameVersion();
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
        if ((index * sizeof(int16_t)) < res->imageDataSize) {
            ((int16_t*)res->imageData)[index] = valueToWrite;
        }
    }
}

void WriteTextureDataInt16ByCrc(uint64_t crc, size_t index, int16_t valueToWrite) {
    const auto res = static_pointer_cast<Ship::Texture>(LoadResource(crc));

    if (res != nullptr) {
        if ((index * sizeof(int16_t)) < res->imageDataSize) {
            ((int16_t*)res->imageData)[index] = valueToWrite;
        }
    }
}
}
