#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <queue>
#include <variant>
#include "resource/Resource.h"
#include "resource/ResourceLoader.h"
#include "resource/archive/ArchiveManager.h"

#define BS_THREAD_POOL_ENABLE_PRIORITY
#define BS_THREAD_POOL_ENABLE_PAUSE
#include <BS_thread_pool.hpp>

namespace Ship {
struct File;

// Resource manager caches any and all files it comes across into memory. This will be unoptimal in the future when
// modifications have gigabytes of assets. It works with the original game's assets because the entire ROM is 64MB and
// fits into RAM of any semi-modern PC.
class ResourceManager {
    typedef enum class ResourceLoadError { None, NotCached, NotFound } ResourceLoadError;

  public:
    ResourceManager();
    void Init(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
              int32_t reservedThreadCount = 1);
    ~ResourceManager();

    bool DidLoadSuccessfully();
    std::shared_ptr<ArchiveManager> GetArchiveManager();
    std::shared_ptr<ResourceLoader> GetResourceLoader();
    std::shared_ptr<Ship::IResource> GetCachedResource(const std::string& filePath, bool loadExact = false);
    std::shared_ptr<Ship::IResource> LoadResource(const std::string& filePath, bool loadExact = false,
                                                  std::shared_ptr<Ship::ResourceInitData> initData = nullptr);
    std::shared_ptr<Ship::IResource> LoadResourceProcess(const std::string& filePath, bool loadExact = false,
                                                         std::shared_ptr<Ship::ResourceInitData> initData = nullptr);
    size_t UnloadResource(const std::string& filePath);
    std::shared_future<std::shared_ptr<Ship::IResource>>
    LoadResourceAsync(const std::string& filePath, bool loadExact = false, BS::priority_t priority = BS::pr::normal,
                      std::shared_ptr<Ship::ResourceInitData> initData = nullptr);
    std::shared_ptr<std::vector<std::shared_ptr<Ship::IResource>>> LoadDirectory(const std::string& searchMask);
    std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<Ship::IResource>>>>
    LoadDirectoryAsync(const std::string& searchMask, BS::priority_t priority = BS::pr::normal);
    void DirtyDirectory(const std::string& searchMask);
    void UnloadDirectory(const std::string& searchMask);
    bool OtrSignatureCheck(const char* fileName);
    bool IsAltAssetsEnabled();
    void SetAltAssetsEnabled(bool isEnabled);

  protected:
    std::shared_ptr<Ship::File> LoadFileProcess(const std::string& filePath,
                                                std::shared_ptr<Ship::ResourceInitData> initData = nullptr);
    std::shared_ptr<Ship::IResource>
    GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<Ship::IResource>> cacheLine);
    std::variant<ResourceLoadError, std::shared_ptr<Ship::IResource>> CheckCache(const std::string& filePath,
                                                                                 bool loadExact = false);

  private:
    std::unordered_map<std::string, std::variant<ResourceLoadError, std::shared_ptr<Ship::IResource>>> mResourceCache;
    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<ArchiveManager> mArchiveManager;
    std::shared_ptr<BS::thread_pool> mThreadPool;
    std::mutex mMutex;
    bool mAltAssetsEnabled = false;
};
} // namespace Ship
