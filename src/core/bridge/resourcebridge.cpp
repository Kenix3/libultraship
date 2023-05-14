#include "core/bridge/resourcebridge.h"
#include "core/Context.h"
#include <string>
#include <algorithm>
#include <StrHash64.h>

std::shared_ptr<LUS::Resource> LoadResource(const char* name) {
    return LUS::Context::GetInstance()->GetResourceManager()->LoadResource(name);
}

std::shared_ptr<LUS::Resource> LoadResource(uint64_t crc) {
    auto name = GetResourceNameByCrc(crc);

    if (name == nullptr || strlen(name) == 0) {
        SPDLOG_TRACE("LoadResource: Unknown crc {}\n", crc);
        return nullptr;
    }

    return LoadResource(name);
}

extern "C" {

uint64_t GetResourceCrcByName(const char* name) {
    return CRC64(name);
}

const char* GetResourceNameByCrc(uint64_t crc) {
    const std::string* hashStr = LUS::Context::GetInstance()->GetResourceManager()->GetArchive()->HashToString(crc);
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

uint8_t GetResourceIsCustomByName(const char* name) {
    auto resource = LoadResource(name);

    if (resource == nullptr) {
        return false;
    }

    return resource->InitData->IsCustom;
}

uint8_t GetResourceIsCustomByCrc(uint64_t crc) {
    return GetResourceIsCustomByName(GetResourceNameByCrc(crc));
}

void* GetResourceDataByName(const char* name) {
    auto resource = LoadResource(name);

    if (resource == nullptr) {
        return nullptr;
    }

    return resource->GetPointer();
}

void* GetResourceDataByCrc(uint64_t crc) {
    auto name = GetResourceNameByCrc(crc);

    if (name == nullptr || strlen(name) == 0) {
        SPDLOG_TRACE("GetResourceDataByCrc: Unknown crc {}\n", crc);
        return nullptr;
    }

    return GetResourceDataByName(name);
}

uint16_t GetResourceTexWidthByName(const char* name) {
    const auto res = static_pointer_cast<LUS::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexWidthByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<LUS::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexHeightByName(const char* name) {
    const auto res = static_pointer_cast<LUS::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t GetResourceTexHeightByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<LUS::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t GetResourceTexSizeByName(const char* name) {
    const auto res = static_pointer_cast<LUS::Texture>(LoadResource(name));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t GetResourceTexSizeByCrc(uint64_t crc) {
    const auto res = static_pointer_cast<LUS::Texture>(LoadResource(crc));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

void GetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount) {
    auto list = LUS::Context::GetInstance()->GetResourceManager()->GetArchive()->GetGameVersions();
    memcpy(versions, list.data(), std::min(versionsSize, list.size() * sizeof(uint32_t)));
    *versionsCount = list.size();
}

void LoadResourceDirectoryAsync(const char* name) {
    LUS::Context::GetInstance()->GetResourceManager()->LoadDirectoryAsync(name);
}

uint32_t HasGameVersion(uint32_t hash) {
    auto list = LUS::Context::GetInstance()->GetResourceManager()->GetArchive()->GetGameVersions();
    return std::find(list.begin(), list.end(), hash) != list.end();
}

void LoadResourceDirectory(const char* name) {
    LUS::Context::GetInstance()->GetResourceManager()->LoadDirectory(name);
}

void DirtyResourceDirectory(const char* name) {
    LUS::Context::GetInstance()->GetResourceManager()->DirtyDirectory(name);
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

void UnloadResourceByName(const char* name) {
    LUS::Context::GetInstance()->GetResourceManager()->UnloadResource(name);
}

void UnloadResourceByCrc(uint64_t crc) {
    UnloadResourceByName(GetResourceNameByCrc(crc));
}

void UnloadResourceDirectory(const char* name) {
    LUS::Context::GetInstance()->GetResourceManager()->UnloadDirectory(name);
}

uint32_t DoesOtrFileExist() {
    return LUS::Context::GetInstance()->GetResourceManager()->DidLoadSuccessfully();
}
}
