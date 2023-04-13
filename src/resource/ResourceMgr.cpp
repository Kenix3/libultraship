#include "ResourceMgr.h"
#include <spdlog/spdlog.h>
#include "OtrFile.h"
#include "Archive.h"
#include "GameVersions.h"
#include <algorithm>
#include <thread>
#include <Utils/StringHelper.h>
#include <StormLib.h>
#include "core/bridge/consolevariablebridge.h"

// Comes from stormlib. May not be the most efficient, but it's also important to be consistent.
extern bool SFileCheckWildCard(const char* szString, const char* szWildCard);

namespace Ship {

ResourceMgr::ResourceMgr(std::shared_ptr<Window> context, const std::string& mainPath, const std::string& patchesPath,
                         const std::unordered_set<uint32_t>& validHashes)
    : mContext(context) {
    mResourceLoader = std::make_shared<ResourceLoader>(context);
    mArchive = std::make_shared<Archive>(mainPath, patchesPath, validHashes, false);
#if defined(__SWITCH__) || defined(__WIIU__)
    size_t threadCount = 1;
#else
    size_t threadCount = std::min(1U, std::thread::hardware_concurrency() - 1);
#endif
    mThreadPool = std::make_shared<BS::thread_pool>(threadCount);

    if (!DidLoadSuccessfully()) {
        // Nothing ever unpauses the thread pool since nothing will ever try to load the archive again.
        mThreadPool->pause();
    }
}

ResourceMgr::ResourceMgr(std::shared_ptr<Window> context, const std::vector<std::string>& otrFiles,
                         const std::unordered_set<uint32_t>& validHashes)
    : mContext(context) {
    mResourceLoader = std::make_shared<ResourceLoader>(context);
    mArchive = std::make_shared<Archive>(otrFiles, validHashes, false);
#if defined(__SWITCH__) || defined(__WIIU__)
    size_t threadCount = 1;
#else
    size_t threadCount = std::min(1U, std::thread::hardware_concurrency() - 1);
#endif
    mThreadPool = std::make_shared<BS::thread_pool>(threadCount);

    if (!DidLoadSuccessfully()) {
        // Nothing ever unpauses the thread pool since nothing will ever try to load the archive again.
        mThreadPool->pause();
    }
}

ResourceMgr::~ResourceMgr() {
    SPDLOG_INFO("destruct ResourceMgr");
}

bool ResourceMgr::DidLoadSuccessfully() {
    return mArchive != nullptr && mArchive->IsMainMPQValid();
}

std::shared_ptr<OtrFile> ResourceMgr::LoadFileProcess(const std::string& filePath) {
    auto file = mArchive->LoadFile(filePath, true);
    if (file != nullptr) {
        SPDLOG_TRACE("Loaded File {} on ResourceMgr", file->Path);
    } else {
        SPDLOG_WARN("Could not load File {} in ResourceMgr", filePath);
    }
    return file;
}

std::shared_ptr<Resource> ResourceMgr::LoadResourceProcess(const std::string& filePath, bool loadExact) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        const auto newFilePath = filePath.substr(7);
        return LoadResourceProcess(newFilePath);
    }

    // Attempt to load the HD version of the asset, if we fail then we continue trying to load the standard asset.
    if (!loadExact && CVarGetInteger("gHdAssets", 0) && filePath.substr(0, 3) != "hd/") {
        const auto hdPath = "hd/" + filePath;
        auto hdResource = LoadResourceProcess(hdPath, loadExact);

        if (hdResource != nullptr) {
            return hdResource;
        }
    }

    // While waiting in the queue, another thread could have loaded the resource.
    // In a last attempt to avoid doing work that will be discarded, let's check if the cached version exists.
    auto cacheLine = CheckCache(filePath, loadExact);
    auto cachedResource = GetCachedResource(cacheLine);
    if (cachedResource != nullptr) {
        return cachedResource;
    }

    // If we are attempting to load an HD asset, we can return null
    if (!loadExact && CVarGetInteger("gHdAssets", 0) && filePath.substr(0, 3) == "hd/") {
        if (std::holds_alternative<ResourceLoadError>(cacheLine)) {
            try {
                // If we have attempted to cache an HD asset, but failed, we return nullptr and rely on the calling
                // function to return a SD asset. If we have NOT attempted load already, attempt the load.
                auto loadError = std::get<ResourceLoadError>(cacheLine);
                if (loadError != ResourceLoadError::NotCached) {
                    return nullptr;
                }
            } catch (std::bad_variant_access const& e) {
                // Ignore the exception. This should never happen. The last check should've returned the resource.
            }
        }
    }

    // Get the file from the OTR
    auto file = LoadFileProcess(filePath);
    // Transform the raw data into a resource
    auto resource = GetResourceLoader()->LoadResource(file);

    // Another thread could have loaded the resource while we were processing, so we want to check before setting to
    // the cache.
    cachedResource = GetCachedResource(filePath, true);
    {
        const std::lock_guard<std::mutex> lock(mMutex);

        if (cachedResource != nullptr) {
            // If another thread has already loaded this resource, discard the work we already did and return from
            // cache.
            resource = cachedResource;
        }

        // Set the cache to the loaded resource
        if (resource != nullptr) {
            mResourceCache[filePath] = resource;
        } else {
            mResourceCache[filePath] = ResourceLoadError::NotFound;
        }
    }

    if (resource != nullptr) {
        SPDLOG_TRACE("Loaded Resource {} on ResourceMgr", filePath);
    } else {
        SPDLOG_WARN("Resource load FAILED {} on ResourceMgr", filePath);
    }

    return resource;
}

std::vector<uint32_t> ResourceMgr::GetGameVersions() {
    return mArchive->GetGameVersions();
}

void ResourceMgr::PushGameVersion(uint32_t newGameVersion) {
    mArchive->PushGameVersion(newGameVersion);
}

std::shared_future<std::shared_ptr<OtrFile>> ResourceMgr::LoadFileAsync(const std::string& filePath) {
    return mThreadPool->submit(&ResourceMgr::LoadFileProcess, this, filePath).share();
}

std::shared_ptr<OtrFile> ResourceMgr::LoadFile(const std::string& filePath) {
    return LoadFileAsync(filePath).get();
}

std::shared_future<std::shared_ptr<Resource>> ResourceMgr::LoadResourceAsync(const std::string& filePath) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        auto newFilePath = filePath.substr(7);
        return LoadResourceAsync(newFilePath);
    }

    // Check the cache before queueing the job.
    auto cacheCheck = GetCachedResource(filePath);
    if (cacheCheck) {
        auto promise = std::make_shared<std::promise<std::shared_ptr<Resource>>>();
        promise->set_value(cacheCheck);
        return promise->get_future().share();
    }

    const auto newFilePath = std::string(filePath);

    return mThreadPool->submit(&ResourceMgr::LoadResourceProcess, this, newFilePath, false);
}

std::shared_ptr<Resource> ResourceMgr::LoadResource(const std::string& filePath) {
    return LoadResourceAsync(filePath).get();
}

std::variant<ResourceMgr::ResourceLoadError, std::shared_ptr<Resource>>
ResourceMgr::CheckCache(const std::string& filePath, bool loadExact) {
    if (!loadExact && CVarGetInteger("gHdAssets", 0) && filePath.substr(0, 3) != "hd/") {
        const auto hdPath = "hd/" + filePath;
        auto hdCacheResult = CheckCache(hdPath, loadExact);

        // If the type held at this cache index is a resource, then we return it.
        // Else we attempt to load standard definition assets.
        if (std::holds_alternative<std::shared_ptr<Resource>>(hdCacheResult)) {
            return hdCacheResult;
        }
    }

    const std::lock_guard<std::mutex> lock(mMutex);

    auto resourceCacheFind = mResourceCache.find(filePath);
    if (resourceCacheFind == mResourceCache.end()) {
        return ResourceLoadError::NotCached;
    }

    return resourceCacheFind->second;
}

std::shared_ptr<Resource> ResourceMgr::GetCachedResource(const std::string& filePath, bool loadExact) {
    // Gets the cached resource based on filePath.
    return GetCachedResource(CheckCache(filePath, loadExact));
}

std::shared_ptr<Resource>
ResourceMgr::GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<Resource>> cacheLine) {
    // Gets the cached resource based on a cache line std::variant from the cache map.
    if (std::holds_alternative<std::shared_ptr<Resource>>(cacheLine)) {
        try {
            auto resource = std::get<std::shared_ptr<Resource>>(cacheLine);

            if (resource.use_count() <= 0) {
                return nullptr;
            }

            if (resource->IsDirty) {
                return nullptr;
            }

            return resource;
        } catch (std::bad_variant_access const& e) {
            // Ignore the exception
        }
    }

    return nullptr;
}

std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<Resource>>>>
ResourceMgr::LoadDirectoryAsync(const std::string& searchMask) {
    auto loadedList = std::make_shared<std::vector<std::shared_future<std::shared_ptr<Resource>>>>();
    auto fileList = GetArchive()->ListFiles(searchMask);
    loadedList->reserve(fileList->size());

    for (size_t i = 0; i < fileList->size(); i++) {
        auto fileName = std::string(fileList->operator[](i));
        auto future = LoadResourceAsync(fileName);
        loadedList->push_back(future);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::shared_ptr<Resource>>> ResourceMgr::LoadDirectory(const std::string& searchMask) {
    auto futureList = LoadDirectoryAsync(searchMask);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<Resource>>>();

    for (size_t i = 0; i < futureList->size(); i++) {
        const auto future = futureList->at(i);
        const auto resource = future.get();
        loadedList->push_back(resource);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::string>> ResourceMgr::FindLoadedFiles(const std::string& searchMask) {
    const char* wildCard = searchMask.c_str();
    auto list = std::make_shared<std::vector<std::string>>();

    for (const auto& [key, value] : mResourceCache) {
        if (SFileCheckWildCard(key.c_str(), wildCard)) {
            list->push_back(key);
        }
    }

    return list;
}

void ResourceMgr::DirtyDirectory(const std::string& searchMask) {
    auto list = FindLoadedFiles(searchMask);

    for (const auto& key : *list.get()) {
        auto resource = GetCachedResource(key);
        // If it's a resource, we will set the dirty flag, else we will just unload it.
        if (resource != nullptr) {
            resource->IsDirty = true;
        } else {
            UnloadResource(key);
        }
    }
}

void ResourceMgr::UnloadDirectory(const std::string& searchMask) {
    auto list = FindLoadedFiles(searchMask);

    for (const auto& key : *list.get()) {
        UnloadResource(key);
    }
}

const std::string* ResourceMgr::HashToString(uint64_t hash) {
    return mArchive->HashToString(hash);
}

std::shared_ptr<Archive> ResourceMgr::GetArchive() {
    return mArchive;
}

std::shared_ptr<ResourceLoader> ResourceMgr::GetResourceLoader() {
    return mResourceLoader;
}

std::shared_ptr<Window> ResourceMgr::GetContext() {
    return mContext;
}

size_t ResourceMgr::UnloadResource(const std::string& filePath) {
    // Store a shared pointer here so that the erase doesn't destruct the resource.
    // The resource will attempt to load other resources on the destructor, and this will fail because we already hold
    // the mutex.
    std::variant<ResourceLoadError, std::shared_ptr<Resource>> value = nullptr;
    size_t ret = 0;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        value = mResourceCache[filePath];
        ret = mResourceCache.erase(filePath);
    }

    return ret;
}

bool ResourceMgr::OtrSignatureCheck(const char* fileName) {
    static const char* sOtrSignature = "__OTR__";
    return strncmp(fileName, sOtrSignature, strlen(sOtrSignature)) == 0;
}

} // namespace Ship
