#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <list>
#include <vector>
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

struct ResourceFilter {
    ResourceFilter(const std::list<std::string>& includeMasks, const std::list<std::string>& excludeMasks,
                   const uintptr_t owner, const std::shared_ptr<Archive> parent);

    const std::list<std::string> IncludeMasks;
    const std::list<std::string> ExcludeMasks;
    const uintptr_t Owner = 0;
    const std::shared_ptr<Archive> Parent = nullptr;
};

struct ResourceIdentifier {
    friend struct ResourceIdentifierHash;

    ResourceIdentifier(const std::string& path, const uintptr_t owner, const std::shared_ptr<Archive> parent);
    bool operator==(const ResourceIdentifier& rhs) const;

    // Must be an exact path. Passing a path with a wildcard will return a fail state
    const std::string Path = "";
    const uintptr_t Owner = 0;
    const std::shared_ptr<Archive> Parent = nullptr;

  private:
    size_t GetHash() const;
    size_t CalculateHash();
    size_t mHash;
};

struct ResourceIdentifierHash {
    size_t operator()(const ResourceIdentifier& rcd) const;
};

class ResourceManager {
    typedef enum class ResourceLoadError { None, NotCached, NotFound } ResourceLoadError;

  public:
    ResourceManager();
    void Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
              int32_t reservedThreadCount = 1);
    ~ResourceManager();

    bool IsLoaded();

    std::shared_ptr<ArchiveManager> GetArchiveManager();
    std::shared_ptr<ResourceLoader> GetResourceLoader();

    std::shared_ptr<IResource> GetCachedResource(const std::string& filePath, bool loadExact = false);
    std::shared_ptr<IResource> GetCachedResource(const ResourceIdentifier& identifier, bool loadExact = false);
    std::shared_ptr<IResource> LoadResource(const std::string& filePath, bool loadExact = false,
                                            std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<IResource> LoadResource(const ResourceIdentifier& identifier, bool loadExact = false,
                                            std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<IResource> LoadResourceProcess(const std::string& filePath, bool loadExact = false,
                                                   std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<IResource> LoadResourceProcess(const ResourceIdentifier& identifier, bool loadExact = false,
                                                   std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_future<std::shared_ptr<IResource>>
    LoadResourceAsync(const std::string& filePath, bool loadExact = false, BS::priority_t priority = BS::pr::normal,
                      std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_future<std::shared_ptr<IResource>>
    LoadResourceAsync(const ResourceIdentifier& identifier, bool loadExact = false,
                      BS::priority_t priority = BS::pr::normal, std::shared_ptr<ResourceInitData> initData = nullptr);
    size_t UnloadResource(const ResourceIdentifier& identifier);
    size_t UnloadResource(const std::string& filePath);

    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadResources(const std::string& searchMask);
    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadResources(const ResourceFilter& filter);
    std::shared_future<std::shared_ptr<std::vector<std::shared_ptr<IResource>>>>
    LoadResourcesAsync(const std::string& searchMask, BS::priority_t priority = BS::pr::normal);
    std::shared_future<std::shared_ptr<std::vector<std::shared_ptr<IResource>>>>
    LoadResourcesAsync(const ResourceFilter& filter, BS::priority_t priority = BS::pr::normal);

    void DirtyResources(const std::string& searchMask);
    void DirtyResources(const ResourceFilter& filter);
    void UnloadResources(const std::string& searchMask);
    void UnloadResources(const ResourceFilter& filter);
    void UnloadResourcesAsync(const std::string& searchMask, BS::priority_t priority = BS::pr::normal);
    void UnloadResourcesAsync(const ResourceFilter& filter, BS::priority_t priority = BS::pr::normal);

    bool OtrSignatureCheck(const char* fileName);
    bool IsAltAssetsEnabled();
    void SetAltAssetsEnabled(bool isEnabled);

  protected:
    std::shared_ptr<std::vector<std::shared_ptr<IResource>>> LoadResourcesProcess(const ResourceFilter& filter);
    void UnloadResourcesProcess(const ResourceFilter& filter);
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> CheckCache(const ResourceIdentifier& identifier,
                                                                           bool loadExact = false);
    std::shared_ptr<File> LoadFileProcess(const ResourceIdentifier& identifier,
                                          std::shared_ptr<ResourceInitData> initData = nullptr);
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> CheckCache(const std::string& filePath,
                                                                           bool loadExact = false);

    std::shared_ptr<File> LoadFileProcess(const std::string& filePath,
                                          std::shared_ptr<ResourceInitData> initData = nullptr);
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
