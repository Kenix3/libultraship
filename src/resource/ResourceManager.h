#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <queue>
#include <variant>
#include "resource/Resource.h"
#include "resource/ResourceLoader.h"
#include "resource/archive/Archive.h"
#include "resource/archive/ArchiveManager.h"

#define BS_THREAD_POOL_ENABLE_PRIORITY
#define BS_THREAD_POOL_ENABLE_PAUSE

#include <BS_thread_pool.hpp>

namespace Ship {
struct File;

struct ResourceCacheData {
    // Path can either be a Path or a Search Mask including globs depending on usage.
    const std::string Path = "";
    const uintptr_t Owner = 0;
    const std::shared_ptr<Archive> Archive = nullptr;

    std::size_t operator()(const ResourceCacheData& rcd) const;
    bool operator==(const ResourceCacheData& rhs) const;
};

class ResourceManager {
    typedef enum class ResourceLoadError { None, NotCached, NotFound } ResourceLoadError;

  public:
    ResourceManager();
    void Init(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
              int32_t reservedThreadCount = 1);
    ~ResourceManager();

    std::shared_ptr<ArchiveManager> GetArchiveManager();
    std::shared_ptr<ResourceLoader> GetResourceLoader();

    std::shared_ptr<IResource> GetCachedResource(const ResourceCacheData& cacheData, bool loadExact = false);
    std::shared_ptr<IResource> LoadResource(const ResourceCacheData& cacheData, bool loadExact = false, std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<IResource> LoadResourceProcess(const ResourceCacheData& cacheData, bool loadExact = false, std::shared_ptr<ResourceInitData> initData = nullptr);
    size_t UnloadResource(const ResourceCacheData& cacheData);
    std::shared_future<std::shared_ptr<IResource>>
    LoadResourceAsync(const ResourceCacheData& cacheData, bool loadExact = false, BS::priority_t priority = BS::pr::normal, std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadDirectory(const ResourceCacheData& cacheData);
    std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<IResource>>>>
    LoadDirectoryAsync(const ResourceCacheData& cacheData, BS::priority_t priority = BS::pr::normal);
    void DirtyDirectory(const ResourceCacheData& cacheData);
    void UnloadDirectory(const ResourceCacheData& cacheData);

    std::shared_ptr<IResource> GetCachedResource(const std::string& filePath, bool loadExact = false);
    std::shared_ptr<IResource> LoadResource(const std::string& filePath, bool loadExact = false, std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<IResource> LoadResourceProcess(const std::string& filePath, bool loadExact = false, std::shared_ptr<ResourceInitData> initData = nullptr);
    size_t UnloadResource(const std::string& filePath);
    std::shared_future<std::shared_ptr<IResource>>
    LoadResourceAsync(const std::string& filePath, bool loadExact = false, BS::priority_t priority = BS::pr::normal, std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadDirectory(const std::string& searchMask);
    std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<IResource>>>>
    LoadDirectoryAsync(const std::string& searchMask, BS::priority_t priority = BS::pr::normal);
    void DirtyDirectory(const std::string& searchMask);
    void UnloadDirectory(const std::string& searchMask);

    bool DidLoadSuccessfully();
    bool OtrSignatureCheck(const char* fileName);
    bool IsAltAssetsEnabled();
    void SetAltAssetsEnabled(bool isEnabled);

  protected:
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> CheckCache(const ResourceCacheData& cacheData,
                                                                           bool loadExact = false);
    std::shared_ptr<File> LoadFileProcess(const ResourceCacheData& cacheData,
                                          std::shared_ptr<ResourceInitData> initData = nullptr);
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> CheckCache(const std::string& filePath,
                                                                           bool loadExact = false);

    std::shared_ptr<File> LoadFileProcess(const std::string& filePath,
                                          std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<IResource> GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<IResource>> cacheLine);

  private:
    std::unordered_map<ResourceCacheData, std::variant<ResourceLoadError, std::shared_ptr<IResource>>> mResourceCache;
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
