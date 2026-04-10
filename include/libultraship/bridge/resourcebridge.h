#pragma once

#include "stdint.h"
#include "ship/Api.h"

#ifdef __cplusplus
#include "fast/resource/type/Texture.h"
#include "ship/resource/Resource.h"
#include <memory>

std::shared_ptr<Ship::IResource> ResourceLoad(const char* name);
std::shared_ptr<Ship::IResource> ResourceLoad(uint64_t crc);
template <class T> std::shared_ptr<T> ResourceLoad(const char* name) {
    return std::static_pointer_cast<T>(ResourceLoad(name));
}
template <class T> std::shared_ptr<T> ResourceLoad(uint64_t crc) {
    return std::static_pointer_cast<T>(ResourceLoad(crc));
}

extern "C" {
#endif

API_EXPORT uint64_t ResourceGetCrcByName(const char* name);
API_EXPORT const char* ResourceGetNameByCrc(uint64_t crc);
API_EXPORT size_t ResourceGetSizeByName(const char* name);
API_EXPORT size_t ResourceGetSizeByCrc(uint64_t crc);
API_EXPORT uint8_t ResourceGetIsCustomByName(const char* name);
API_EXPORT uint8_t ResourceGetIsCustomByCrc(uint64_t crc);
API_EXPORT void* ResourceGetDataByName(const char* name);
API_EXPORT void* ResourceGetDataByCrc(uint64_t crc);
API_EXPORT uint16_t ResourceGetTexWidthByName(const char* name);
API_EXPORT uint16_t ResourceGetTexWidthByCrc(uint64_t crc);
API_EXPORT uint16_t ResourceGetTexHeightByName(const char* name);
API_EXPORT uint16_t ResourceGetTexHeightByCrc(uint64_t crc);
API_EXPORT size_t ResourceGetTexSizeByName(const char* name);
API_EXPORT size_t ResourceGetTexSizeByCrc(uint64_t crc);
API_EXPORT void ResourceLoadDirectory(const char* name);
API_EXPORT void ResourceLoadDirectoryAsync(const char* name);
API_EXPORT void ResourceDirtyDirectory(const char* name);
API_EXPORT void ResourceDirtyByName(const char* name);
API_EXPORT void ResourceDirtyByCrc(uint64_t crc);
API_EXPORT void ResourceUnloadByName(const char* name);
API_EXPORT void ResourceUnloadByCrc(uint64_t crc);
API_EXPORT void ResourceUnloadDirectory(const char* name);
API_EXPORT void ResourceClearCache();
API_EXPORT void ResourceGetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount);
API_EXPORT uint32_t ResourceHasGameVersion(uint32_t hash);
API_EXPORT uint32_t IsResourceManagerLoaded();

#ifdef __cplusplus
};
#endif
