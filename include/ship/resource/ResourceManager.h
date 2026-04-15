#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <list>
#include <vector>
#include <mutex>
#include <queue>
#include <variant>
#include "ship/resource/Resource.h"
#include "ship/resource/ResourceLoader.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/archive/ArchiveManager.h"

#define BS_THREAD_POOL_ENABLE_PRIORITY
#define BS_THREAD_POOL_ENABLE_PAUSE
#include <BS_thread_pool.hpp>

namespace Ship {
struct File;

/**
 * @brief Specifies which resources to include or exclude when performing a bulk load/unload.
 *
 * IncludeMasks and ExcludeMasks are glob-style patterns matched against resource paths.
 * Owner and Parent scope the query to resources loaded by a specific owner or from a
 * specific archive (pass 0 / nullptr to match all).
 */
struct ResourceFilter {
    ResourceFilter(const std::list<std::string>& includeMasks, const std::list<std::string>& excludeMasks,
                   const uintptr_t owner, const std::shared_ptr<Archive> parent);

    /** @brief Glob patterns that a resource path must match to be included. */
    const std::list<std::string> IncludeMasks;
    /** @brief Glob patterns that cause a matching resource to be excluded even when it matches IncludeMasks. */
    const std::list<std::string> ExcludeMasks;
    /** @brief Opaque owner handle; 0 matches resources from any owner. */
    const uintptr_t Owner = 0;
    /** @brief Archive to scope the query to; nullptr matches resources from any archive. */
    const std::shared_ptr<Archive> Parent = nullptr;
};

/**
 * @brief Uniquely identifies a cached resource by path, owner, and source archive.
 *
 * Two ResourceIdentifiers compare equal only when all three fields match, which allows
 * the same logical path to be loaded from different archives or by different owners
 * without colliding in the resource cache.
 */
struct ResourceIdentifier {
    friend struct ResourceIdentifierHash;

    ResourceIdentifier(const std::string& path, const uintptr_t owner, const std::shared_ptr<Archive> parent);
    ResourceIdentifier(std::string&& path, const uintptr_t owner, const std::shared_ptr<Archive> parent);
    bool operator==(const ResourceIdentifier& rhs) const;

    // Must be an exact path. Passing a path with a wildcard will return a fail state
    const std::string Path = "";
    /** @brief Opaque handle that identifies the owner of this cache entry. */
    const uintptr_t Owner = 0;
    /** @brief The archive from which the resource was (or should be) loaded. */
    const std::shared_ptr<Archive> Parent = nullptr;

  private:
    size_t GetHash() const;
    size_t CalculateHash();
    size_t mHash;
};

/**
 * @brief std::hash specialization for ResourceIdentifier, used by unordered containers.
 */
struct ResourceIdentifierHash {
    size_t operator()(const ResourceIdentifier& rcd) const;
};

/**
 * @brief Central manager for loading, caching, and unloading game resources.
 *
 * ResourceManager sits between the game code and the archive/file system. It maintains
 * an in-memory cache of loaded IResource objects, dispatches asynchronous load requests
 * to a thread pool, and delegates actual deserialization to ResourceLoader.
 *
 * Typical usage:
 * @code
 * auto rm = Ship::Context::GetInstance()->GetResourceManager();
 * auto tex = rm->LoadResource<Ship::Texture>("textures/foo.tex");
 * @endcode
 */
class ResourceManager {
    friend class ResourceLoader;
    typedef enum class ResourceLoadError { None, NotCached, NotFound } ResourceLoadError;

  public:
    ResourceManager();

    /**
     * @brief Initializes the ResourceManager, mounting archives and starting the thread pool.
     * @param archivePaths        Paths to OTR/O2R archive files or directories containing them.
     * @param validHashes         Set of acceptable game-version hash values; empty = all accepted.
     * @param reservedThreadCount Number of OS threads to reserve outside the resource thread pool.
     */
    void Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
              int32_t reservedThreadCount = 1);
    ~ResourceManager();

    /**
     * @brief Returns true once Init() has completed successfully.
     * @return true if the manager is ready to load resources.
     */
    bool IsLoaded();

    /** @brief Returns the ArchiveManager that manages the mounted archives. */
    std::shared_ptr<ArchiveManager> GetArchiveManager();
    /** @brief Returns the ResourceLoader responsible for deserializing file data. */
    std::shared_ptr<ResourceLoader> GetResourceLoader();

    /**
     * @brief Returns a resource from the cache if present, without loading from disk.
     * @param filePath  Virtual path of the resource.
     * @param loadExact If true, skips alt-asset path resolution and uses the exact path.
     * @return Cached IResource, or nullptr if not found in the cache.
     */
    std::shared_ptr<IResource> GetCachedResource(const std::string& filePath, bool loadExact = false);

    /**
     * @brief Returns a resource from the cache using a ResourceIdentifier.
     * @param identifier Exact resource identifier including owner and parent archive.
     * @param loadExact  If true, skips alt-asset path resolution.
     * @return Cached IResource, or nullptr if not found.
     */
    std::shared_ptr<IResource> GetCachedResource(const ResourceIdentifier& identifier, bool loadExact = false);

    /**
     * @brief Loads a resource synchronously, returning it from the cache if already loaded.
     * @param filePath  Virtual path of the resource.
     * @param loadExact If true, skips alt-asset path resolution.
     * @param initData  Optional metadata overrides; pass nullptr to use defaults from the file header.
     * @return Loaded (or cached) IResource, or nullptr on failure.
     */
    std::shared_ptr<IResource> LoadResource(const std::string& filePath, bool loadExact = false,
                                            std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Loads a resource synchronously by ResourceIdentifier.
     * @param identifier Exact identifier (path + owner + parent archive).
     * @param loadExact  If true, skips alt-asset path resolution.
     * @param initData   Optional metadata overrides.
     * @return Loaded (or cached) IResource, or nullptr on failure.
     */
    std::shared_ptr<IResource> LoadResource(const ResourceIdentifier& identifier, bool loadExact = false,
                                            std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Loads a resource synchronously by content-hash CRC.
     * @param crc       64-bit content hash identifying the resource.
     * @param loadExact If true, skips alt-asset path resolution.
     * @param initData  Optional metadata overrides.
     * @return Loaded IResource, or nullptr if the CRC is not found in any mounted archive.
     */
    std::shared_ptr<IResource> LoadResource(uint64_t crc, bool loadExact = false,
                                            std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Loads a resource synchronously, bypassing the cache.
     *
     * Unlike LoadResource(), this always reads from the archive even if a cached copy exists.
     *
     * @param filePath  Virtual path of the resource.
     * @param loadExact If true, skips alt-asset path resolution.
     * @param initData  Optional metadata overrides.
     * @return Freshly loaded IResource, or nullptr on failure.
     */
    std::shared_ptr<IResource> LoadResourceProcess(const std::string& filePath, bool loadExact = false,
                                                   std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Loads a resource synchronously by ResourceIdentifier, bypassing the cache.
     * @param identifier Exact identifier.
     * @param loadExact  If true, skips alt-asset path resolution.
     * @param initData   Optional metadata overrides.
     * @return Freshly loaded IResource, or nullptr on failure.
     */
    std::shared_ptr<IResource> LoadResourceProcess(const ResourceIdentifier& identifier, bool loadExact = false,
                                                   std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Schedules an asynchronous resource load and returns a future.
     * @param filePath  Virtual path of the resource.
     * @param loadExact If true, skips alt-asset path resolution.
     * @param priority  Thread-pool scheduling priority.
     * @param initData  Optional metadata overrides.
     * @return Shared future that will resolve to the loaded IResource (or nullptr on failure).
     */
    std::shared_future<std::shared_ptr<IResource>>
    LoadResourceAsync(const std::string& filePath, bool loadExact = false, BS::priority_t priority = BS::pr::normal,
                      std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Schedules an asynchronous resource load by ResourceIdentifier.
     * @param identifier Exact identifier.
     * @param loadExact  If true, skips alt-asset path resolution.
     * @param priority   Thread-pool scheduling priority.
     * @param initData   Optional metadata overrides.
     * @return Shared future that will resolve to the loaded IResource (or nullptr on failure).
     */
    std::shared_future<std::shared_ptr<IResource>>
    LoadResourceAsync(const ResourceIdentifier& identifier, bool loadExact = false,
                      BS::priority_t priority = BS::pr::normal, std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Removes a resource from the cache by ResourceIdentifier.
     * @param identifier Identifier of the resource to evict.
     * @return Number of cache entries removed (0 or 1).
     */
    size_t UnloadResource(const ResourceIdentifier& identifier);

    /**
     * @brief Removes a resource from the cache by path.
     * @param filePath Virtual path of the resource to evict.
     * @return Number of cache entries removed (0 or 1).
     */
    size_t UnloadResource(const std::string& filePath);

    /**
     * @brief Writes raw data into an archive and optionally evicts the stale cache entry.
     * @param identifier Identifier of the resource to write.
     * @param data       Raw bytes to write.
     * @param unloadFile If true, removes the old cache entry after writing.
     * @return true on success.
     */
    bool WriteResource(const ResourceIdentifier& identifier, const std::vector<uint8_t>& data, bool unloadFile);

    /**
     * @brief Loads all resources whose paths match the given glob mask.
     * @param searchMask Glob pattern (e.g. "textures/ui/*").
     * @return Pointer to a vector of loaded IResource objects.
     */
    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadResources(const std::string& searchMask);

    /**
     * @brief Loads all resources matching the given filter.
     * @param filter ResourceFilter specifying include/exclude masks, owner, and parent archive.
     * @return Pointer to a vector of loaded IResource objects.
     */
    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadResources(const ResourceFilter& filter);

    /**
     * @brief Asynchronously loads all resources matching a glob mask.
     * @param searchMask Glob pattern.
     * @param priority   Thread-pool scheduling priority.
     * @return Shared future resolving to a vector of loaded IResource objects.
     */
    std::shared_future<std::shared_ptr<std::vector<std::shared_ptr<IResource>>>>
    LoadResourcesAsync(const std::string& searchMask, BS::priority_t priority = BS::pr::normal);

    /**
     * @brief Asynchronously loads all resources matching a filter.
     * @param filter   ResourceFilter.
     * @param priority Thread-pool scheduling priority.
     * @return Shared future resolving to a vector of loaded IResource objects.
     */
    std::shared_future<std::shared_ptr<std::vector<std::shared_ptr<IResource>>>>
    LoadResourcesAsync(const ResourceFilter& filter, BS::priority_t priority = BS::pr::normal);

    /**
     * @brief Marks as dirty all cached resources whose paths match a glob mask.
     *
     * Dirty resources remain in the cache but are flagged for reload on next access.
     *
     * @param searchMask Glob pattern.
     */
    void DirtyResources(const std::string& searchMask);

    /**
     * @brief Marks as dirty all cached resources matching a filter.
     * @param filter ResourceFilter.
     */
    void DirtyResources(const ResourceFilter& filter);

    /**
     * @brief Synchronously unloads all resources matching a glob mask.
     * @param searchMask Glob pattern.
     */
    void UnloadResources(const std::string& searchMask);

    /**
     * @brief Synchronously unloads all resources matching a filter.
     * @param filter ResourceFilter.
     */
    void UnloadResources(const ResourceFilter& filter);

    /**
     * @brief Asynchronously unloads all resources matching a glob mask.
     * @param searchMask Glob pattern.
     * @param priority   Thread-pool scheduling priority.
     */
    void UnloadResourcesAsync(const std::string& searchMask, BS::priority_t priority = BS::pr::normal);

    /**
     * @brief Asynchronously unloads all resources matching a filter.
     * @param filter   ResourceFilter.
     * @param priority Thread-pool scheduling priority.
     */
    void UnloadResourcesAsync(const ResourceFilter& filter, BS::priority_t priority = BS::pr::normal);

    /**
     * @brief Checks whether a file is a valid OTR archive by reading its header signature.
     * @param fileName Path to the file to inspect.
     * @return true if the file starts with a valid OTR signature.
     */
    bool OtrSignatureCheck(const char* fileName);

    /**
     * @brief Returns whether alt-asset (mod) loading is currently enabled.
     * @return true if alternate assets are active.
     */
    bool IsAltAssetsEnabled();

    /**
     * @brief Enables or disables alt-asset (mod) loading.
     * @param isEnabled true to enable, false to disable.
     */
    void SetAltAssetsEnabled(bool isEnabled);

    /**
     * @brief Loads raw file bytes from the archive, bypassing resource deserialization.
     * @param identifier Exact resource identifier.
     * @return Loaded File with raw buffer, or nullptr on failure.
     */
    std::shared_ptr<File> LoadFileProcess(const ResourceIdentifier& identifier);

    /**
     * @brief Loads raw file bytes from the archive by path.
     * @param filePath Virtual path of the file.
     * @return Loaded File with raw buffer, or nullptr on failure.
     */
    std::shared_ptr<File> LoadFileProcess(const std::string& filePath);

    /**
     * @brief Returns the byte size of the payload of a loaded resource.
     * @param resource Shared pointer to the resource.
     * @return Size in bytes, or 0 if the resource is null.
     */
    size_t GetResourceSize(std::shared_ptr<IResource> resource);

    /**
     * @brief Returns the byte size of the payload of a resource identified by path.
     * @param name Virtual path of the resource.
     * @return Size in bytes, or 0 if not found.
     */
    size_t GetResourceSize(const char* name);

    /**
     * @brief Returns the byte size of the payload of a resource identified by CRC.
     * @param crc 64-bit content hash.
     * @return Size in bytes, or 0 if not found.
     */
    size_t GetResourceSize(uint64_t crc);

    /**
     * @brief Returns true if the given resource originated from an alt-assets override.
     * @param resource Shared pointer to the resource.
     */
    bool GetResourceIsCustom(std::shared_ptr<IResource> resource);

    /**
     * @brief Returns true if the resource at the given path is an alt-asset override.
     * @param name Virtual path of the resource.
     */
    bool GetResourceIsCustom(const char* name);

    /**
     * @brief Returns true if the resource identified by CRC is an alt-asset override.
     * @param crc 64-bit content hash.
     */
    bool GetResourceIsCustom(uint64_t crc);

    /**
     * @brief Returns a type-erased raw pointer to the resource payload.
     * @param resource Shared pointer to the resource.
     * @return Void pointer to the payload, or nullptr if the resource is null.
     */
    void* GetResourceRawPointer(std::shared_ptr<IResource> resource);

    /**
     * @brief Returns a type-erased raw pointer to the payload of a resource by path.
     * @param name Virtual path of the resource.
     * @return Void pointer to the payload, or nullptr if not found.
     */
    void* GetResourceRawPointer(const char* name);

    /**
     * @brief Returns a type-erased raw pointer to the payload of a resource by CRC.
     * @param crc 64-bit content hash.
     * @return Void pointer to the payload, or nullptr if not found.
     */
    void* GetResourceRawPointer(uint64_t crc);

  protected:
    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadResourcesProcess(const ResourceFilter& filter);
    void UnloadResourcesProcess(const ResourceFilter& filter);
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> CheckCache(const ResourceIdentifier& identifier,
                                                                           bool loadExact = false);
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> CheckCache(const std::string& filePath,
                                                                           bool loadExact = false);

    std::shared_ptr<IResource> GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<IResource>> cacheLine);

  private:
    std::unordered_map<ResourceIdentifier, std::variant<ResourceLoadError, std::shared_ptr<IResource>>,
                       ResourceIdentifierHash>
        mResourceCache;
    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<ArchiveManager> mArchiveManager;
    std::shared_ptr<BS::thread_pool> mThreadPool;
    std::mutex mMutex;
    bool mAltAssetsEnabled = false;
    // Private information for which owner and archive are default.
    uintptr_t mDefaultCacheOwner = 0;
    std::shared_ptr<Archive> mDefaultCacheArchive = nullptr;
};
} // namespace Ship
