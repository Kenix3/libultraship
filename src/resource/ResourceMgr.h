#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <queue>
#include <variant>
#include "core/Window.h"
#include "Resource.h"
#include "ResourceLoader.h"
#include "Archive.h"
#include "thread-pool/BS_thread_pool.hpp"

namespace Ship {
class Window;
struct OtrFile;

// Resource manager caches any and all files it comes across into memory. This will be unoptimal in the future when
// modifications have gigabytes of assets. It works with the original game's assets because the entire ROM is 64MB and
// fits into RAM of any semi-modern PC.
class ResourceMgr {
    friend class Resource;
    typedef enum class ResourceLoadError { None, NotCached, NotFound } ResourceLoadError;

  public:
    ResourceMgr(std::shared_ptr<Window> context, const std::string& mainPath, const std::string& patchesPath,
                const std::unordered_set<uint32_t>& validHashes);
    ResourceMgr(std::shared_ptr<Window> context, const std::vector<std::string>& otrFiles,
                const std::unordered_set<uint32_t>& validHashes);
    ~ResourceMgr();

    bool DidLoadSuccessfully();
    std::shared_ptr<Archive> GetArchive();
    std::shared_ptr<Window> GetContext();
    std::shared_ptr<ResourceLoader> GetResourceLoader();
    std::vector<uint32_t> GetGameVersions();
    void PushGameVersion(uint32_t newGameVersion);
    std::shared_future<std::shared_ptr<OtrFile>> LoadFileAsync(const std::string& filePath);
    std::shared_ptr<OtrFile> LoadFile(const std::string& filePath);
    std::shared_ptr<Resource> GetCachedResource(const std::string& filePath, bool loadExact = false);
    std::shared_ptr<Resource> LoadResource(const std::string& filePath);
    std::shared_ptr<Resource> LoadResourceProcess(const std::string& filePath, bool loadExact = false);
    size_t UnloadResource(const std::string& filePath);
    std::shared_future<std::shared_ptr<Resource>> LoadResourceAsync(const std::string& filePath);
    std::shared_ptr<std::vector<std::shared_ptr<Resource>>> LoadDirectory(const std::string& searchMask);
    std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<Resource>>>>
    LoadDirectoryAsync(const std::string& searchMask);
    std::shared_ptr<std::vector<std::string>> FindLoadedFiles(const std::string& searchMask);
    void DirtyDirectory(const std::string& searchMask);
    void UnloadDirectory(const std::string& searchMask);
    bool OtrSignatureCheck(const char* fileName);
    const std::string* HashToString(uint64_t hash);

  protected:
    std::shared_ptr<OtrFile> LoadFileProcess(const std::string& filePath);
    std::shared_ptr<Resource> GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<Resource>> cacheLine);
    std::variant<ResourceLoadError, std::shared_ptr<Resource>> CheckCache(const std::string& filePath,
                                                                          bool loadExact = false);

  private:
    std::shared_ptr<Window> mContext;
    std::unordered_map<std::string, std::variant<ResourceLoadError, std::shared_ptr<Resource>>> mResourceCache;
    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<Archive> mArchive;
    std::shared_ptr<BS::thread_pool> mThreadPool;
    std::mutex mMutex;
};
} // namespace Ship
