#pragma once

#include "stdint.h"
#include "ship/Api.h"

#ifdef __cplusplus
#include "fast/resource/type/Texture.h"
#include "ship/resource/Resource.h"
#include <memory>

/**
 * @brief Loads a resource by its virtual path and returns a type-erased shared pointer.
 * @param name Virtual path of the resource within the archive (e.g. "textures/foo.png").
 * @return Shared pointer to the loaded IResource, or nullptr on failure.
 */
std::shared_ptr<Ship::IResource> ResourceLoad(const char* name);

/**
 * @brief Loads a resource by its 64-bit CRC and returns a type-erased shared pointer.
 * @param crc 64-bit CRC of the resource path.
 * @return Shared pointer to the loaded IResource, or nullptr on failure.
 */
std::shared_ptr<Ship::IResource> ResourceLoad(uint64_t crc);

/**
 * @brief Loads a resource by its virtual path and casts it to the requested type.
 * @tparam T  Expected resource type (e.g. Fast::Texture).
 * @param  name Virtual resource path.
 * @return Shared pointer to @p T, or nullptr if the resource could not be loaded or cast.
 */
template <class T> std::shared_ptr<T> ResourceLoad(const char* name) {
    return std::static_pointer_cast<T>(ResourceLoad(name));
}

/**
 * @brief Loads a resource by CRC and casts it to the requested type.
 * @tparam T   Expected resource type.
 * @param  crc 64-bit CRC of the resource path.
 * @return Shared pointer to @p T, or nullptr on failure.
 */
template <class T> std::shared_ptr<T> ResourceLoad(uint64_t crc) {
    return std::static_pointer_cast<T>(ResourceLoad(crc));
}

extern "C" {
#endif

/** @brief Returns the 64-bit CRC that corresponds to the given resource path. */
API_EXPORT uint64_t ResourceGetCrcByName(const char* name);

/** @brief Returns the virtual path string that corresponds to the given 64-bit CRC. */
API_EXPORT const char* ResourceGetNameByCrc(uint64_t crc);

/** @brief Returns the raw data size in bytes of the resource with the given path. */
API_EXPORT size_t ResourceGetSizeByName(const char* name);

/** @brief Returns the raw data size in bytes of the resource with the given CRC. */
API_EXPORT size_t ResourceGetSizeByCrc(uint64_t crc);

/**
 * @brief Returns non-zero if the resource at @p name originates from a custom/mod archive.
 */
API_EXPORT uint8_t ResourceGetIsCustomByName(const char* name);

/**
 * @brief Returns non-zero if the resource at @p crc originates from a custom/mod archive.
 */
API_EXPORT uint8_t ResourceGetIsCustomByCrc(uint64_t crc);

/**
 * @brief Returns a raw pointer to the resource's data buffer by path.
 *
 * The pointer is valid as long as the resource remains loaded; do not cache it
 * across frames without also holding a reference to the resource.
 */
API_EXPORT void* ResourceGetDataByName(const char* name);

/** @brief Returns a raw pointer to the resource's data buffer by CRC. */
API_EXPORT void* ResourceGetDataByCrc(uint64_t crc);

/** @brief Returns the texture width in pixels for the texture resource at @p name. */
API_EXPORT uint16_t ResourceGetTexWidthByName(const char* name);

/** @brief Returns the texture width in pixels for the texture resource at @p crc. */
API_EXPORT uint16_t ResourceGetTexWidthByCrc(uint64_t crc);

/** @brief Returns the texture height in pixels for the texture resource at @p name. */
API_EXPORT uint16_t ResourceGetTexHeightByName(const char* name);

/** @brief Returns the texture height in pixels for the texture resource at @p crc. */
API_EXPORT uint16_t ResourceGetTexHeightByCrc(uint64_t crc);

/** @brief Returns the total texture data size in bytes for the texture at @p name. */
API_EXPORT size_t ResourceGetTexSizeByName(const char* name);

/** @brief Returns the total texture data size in bytes for the texture at @p crc. */
API_EXPORT size_t ResourceGetTexSizeByCrc(uint64_t crc);

/**
 * @brief Synchronously loads all resources whose paths begin with @p name.
 * @param name Directory prefix (e.g. "textures/") or exact path.
 */
API_EXPORT void ResourceLoadDirectory(const char* name);

/**
 * @brief Queues all resources under @p name for background loading.
 * @param name Directory prefix or exact path.
 */
API_EXPORT void ResourceLoadDirectoryAsync(const char* name);

/**
 * @brief Marks all resources under @p name as dirty so they are reloaded on next access.
 * @param name Directory prefix or exact path.
 */
API_EXPORT void ResourceDirtyDirectory(const char* name);

/** @brief Marks the resource at @p name as dirty so it is reloaded on next access. */
API_EXPORT void ResourceDirtyByName(const char* name);

/** @brief Marks the resource at @p crc as dirty so it is reloaded on next access. */
API_EXPORT void ResourceDirtyByCrc(uint64_t crc);

/** @brief Evicts the resource with the given path from the resource cache. */
API_EXPORT void ResourceUnloadByName(const char* name);

/** @brief Evicts the resource with the given CRC from the resource cache. */
API_EXPORT void ResourceUnloadByCrc(uint64_t crc);

/**
 * @brief Evicts all resources whose paths begin with @p name from the resource cache.
 * @param name Directory prefix or exact path.
 */
API_EXPORT void ResourceUnloadDirectory(const char* name);

/** @brief Evicts all resources from the resource cache. */
API_EXPORT void ResourceClearCache();

/**
 * @brief Retrieves the list of game version hashes present in the loaded archives.
 * @param versions      Caller-supplied array to receive the version hashes.
 * @param versionsSize  Capacity of @p versions (number of elements).
 * @param versionsCount Set to the actual number of versions written to @p versions.
 */
API_EXPORT void ResourceGetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount);

/**
 * @brief Returns non-zero if the given version hash is present in the loaded archives.
 * @param hash Version hash to search for.
 */
API_EXPORT uint32_t ResourceHasGameVersion(uint32_t hash);

/** @brief Returns non-zero if the ResourceManager has been fully initialised and is ready. */
API_EXPORT uint32_t IsResourceManagerLoaded();

#ifdef __cplusplus
};
#endif
