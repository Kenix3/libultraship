#pragma once

#include <unordered_map>
#include <string>
#include <thread>
#include <queue>
#include <variant>
#include "core/Window.h"
#include "Resource.h"
#include "ResourceLoader.h"
#include "Archive.h"
#include "OtrFile.h"

namespace Ship {
class Window;

// Resource manager caches any and all files it comes across into memory. This will be unoptimal in the future when
// modifications have gigabytes of assets. It works with the original game's assets because the entire ROM is 64MB and
// fits into RAM of any semi-modern PC.
class ResourceMgr {
  public:
    ResourceMgr(std::shared_ptr<Window> context, const std::string& mainPath, const std::string& patchesPath,
                const std::unordered_set<uint32_t>& validHashes);
    ResourceMgr(std::shared_ptr<Window> context, const std::vector<std::string>& otrFiles,
                const std::unordered_set<uint32_t>& validHashes);
    ~ResourceMgr();

    bool IsRunning();
    bool DidLoadSuccessfully();

    std::shared_ptr<Archive> GetArchive();
    std::shared_ptr<Window> GetContext();
    std::shared_ptr<ResourceLoader> GetResourceLoader();
    const std::string* HashToString(uint64_t hash) const;
    void InvalidateResourceCache();
    std::vector<uint32_t> GetGameVersions();
    void PushGameVersion(uint32_t newGameVersion);
    std::shared_ptr<OtrFile> LoadFileAsync(const std::string& filePath);
    std::shared_ptr<OtrFile> LoadFile(const std::string& filePath);
    std::shared_ptr<Resource> GetCachedFile(const char* filePath) const;
    std::shared_ptr<Resource> LoadResource(const char* filePath);
    std::shared_ptr<Resource> LoadResource(const std::string& filePath);
    std::variant<std::shared_ptr<Resource>, std::shared_ptr<ResourcePromise>> LoadResourceAsync(const char* filePath);
    std::shared_ptr<std::vector<std::shared_ptr<Resource>>> CacheDirectory(const std::string& searchMask);
    std::shared_ptr<std::vector<std::shared_ptr<ResourcePromise>>> CacheDirectoryAsync(const std::string& searchMask);
    std::shared_ptr<std::vector<std::shared_ptr<Resource>>> DirtyDirectory(const std::string& searchMask);
    std::shared_ptr<std::vector<std::string>> ListFiles(std::string searchMask);
    int32_t OtrSignatureCheck(char* imgData);
    size_t UnloadResource(const std::string& filePath);

  protected:
    void Start();
    void Stop();
    void LoadFileThread();
    void LoadResourceThread();

  private:
    std::shared_ptr<Window> mContext;
    volatile bool mIsRunning;
    std::unordered_map<std::string, std::shared_ptr<OtrFile>> mFileCache;
    std::unordered_map<std::string, std::shared_ptr<Resource>> mResourceCache;
    std::queue<std::shared_ptr<OtrFile>> mFileLoadQueue;
    std::queue<std::shared_ptr<ResourcePromise>> mResourceLoadQueue;
    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<Archive> mArchive;
    std::shared_ptr<std::thread> mFileLoadThread;
    std::shared_ptr<std::thread> mResourceLoadThread;
    std::mutex mFileLoadMutex;
    std::mutex mResourceLoadMutex;
    std::condition_variable mFileLoadNotifier;
    std::condition_variable mResourceLoadNotifier;
};
} // namespace Ship
