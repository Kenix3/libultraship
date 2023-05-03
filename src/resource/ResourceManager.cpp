#include "ResourceManager.h"
#include <spdlog/spdlog.h>
#include "File.h"
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

ResourceManager::ResourceManager(std::shared_ptr<Window> context, const std::string& mainPath, const std::string& patchesPath,
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

ResourceManager::ResourceManager(std::shared_ptr<Window> context, const std::vector<std::string>& otrFiles,
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

ResourceManager::~ResourceManager() {
    SPDLOG_INFO("destruct ResourceManager");
}

bool ResourceManager::DidLoadSuccessfully() {
    return mArchive != nullptr && mArchive->IsMainMPQValid();
}

std::shared_ptr<File> ResourceManager::LoadFileProcess(const std::string& filePath) {
    auto file = mArchive->LoadFile(filePath, true);
    if (file != nullptr) {
        SPDLOG_TRACE("Loaded File {} on ResourceManager", file->Path);
    } else {
        SPDLOG_WARN("Could not load File {} in ResourceManager", filePath);
    }
    return file;
}

std::shared_ptr<Resource> ResourceManager::LoadResourceProcess(const std::string& filePath, bool loadExact) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        const auto newFilePath = filePath.substr(7);
        return LoadResourceProcess(newFilePath);
    }

    // Attempt to load the alternate version of the asset, if we fail then we continue trying to load the standard
    // asset.
    if (!loadExact && CVarGetInteger("gAltAssets", 0) && !filePath.starts_with(Resource::gAltAssetPrefix)) {
        const auto altPath = Resource::gAltAssetPrefix + filePath;
        auto altResource = LoadResourceProcess(altPath, loadExact);

        if (altResource != nullptr) {
            return altResource;
        }
    }

    // While waiting in the queue, another thread could have loaded the resource.
    // In a last attempt to avoid doing work that will be discarded, let's check if the cached version exists.
    auto cacheLine = CheckCache(filePath, loadExact);
    auto cachedResource = GetCachedResource(cacheLine);
    if (cachedResource != nullptr) {
        return cachedResource;
    }
    // Check for resource load errors which can indicate an alternate asset.
    // If we are attempting to load an alternate asset, we can return null
    if (!loadExact && CVarGetInteger("gAltAssets", 0) && filePath.starts_with(Resource::gAltAssetPrefix)) {
        if (std::holds_alternative<ResourceLoadError>(cacheLine)) {
            try {
                // If we have attempted to cache an alternate asset, but failed, we return nullptr and rely on the
                // calling function to return a regular asset. If we have NOT attempted load already, attempt the load.
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
    if (file == nullptr) {
        SPDLOG_ERROR("Failed to load resource file at path {}", filePath);
    }

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
        SPDLOG_TRACE("Loaded Resource {} on ResourceManager", filePath);
    } else {
        SPDLOG_WARN("Resource load FAILED {} on ResourceManager", filePath);
    }

    return resource;
}

std::shared_future<std::shared_ptr<File>> ResourceManager::LoadFileAsync(const std::string& filePath) {
    return mThreadPool->submit(&ResourceManager::LoadFileProcess, this, filePath).share();
}

std::shared_ptr<File> ResourceManager::LoadFile(const std::string& filePath) {
    return LoadFileAsync(filePath).get();
}

std::shared_future<std::shared_ptr<Resource>> ResourceManager::LoadResourceAsync(const std::string& filePath,
                                                                             bool loadExact) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        auto newFilePath = filePath.substr(7);
        return LoadResourceAsync(newFilePath, loadExact);
    }

    // Check the cache before queueing the job.
    auto cacheCheck = GetCachedResource(filePath, loadExact);
    if (cacheCheck) {
        auto promise = std::make_shared<std::promise<std::shared_ptr<Resource>>>();
        promise->set_value(cacheCheck);
        return promise->get_future().share();
    }

    const auto newFilePath = std::string(filePath);

    return mThreadPool->submit(&ResourceManager::LoadResourceProcess, this, newFilePath, loadExact);
}

std::shared_ptr<Resource> ResourceManager::LoadResource(const std::string& filePath, bool loadExact) {
    return LoadResourceAsync(filePath, loadExact).get();
}

std::variant<ResourceManager::ResourceLoadError, std::shared_ptr<Resource>>
ResourceManager::CheckCache(const std::string& filePath, bool loadExact) {
    if (!loadExact && CVarGetInteger("gAltAssets", 0) && !filePath.starts_with(Resource::gAltAssetPrefix)) {
        const auto altPath = Resource::gAltAssetPrefix + filePath;
        auto altCacheResult = CheckCache(altPath, loadExact);

        // If the type held at this cache index is a resource, then we return it.
        // Else we attempt to load standard definition assets.
        if (std::holds_alternative<std::shared_ptr<Resource>>(altCacheResult)) {
            return altCacheResult;
        }
    }

    const std::lock_guard<std::mutex> lock(mMutex);

    auto resourceCacheFind = mResourceCache.find(filePath);
    if (resourceCacheFind == mResourceCache.end()) {
        return ResourceLoadError::NotCached;
    }

    return resourceCacheFind->second;
}

std::shared_ptr<Resource> ResourceManager::GetCachedResource(const std::string& filePath, bool loadExact) {
    // Gets the cached resource based on filePath.
    return GetCachedResource(CheckCache(filePath, loadExact));
}

std::shared_ptr<Resource>
ResourceManager::GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<Resource>> cacheLine) {
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
ResourceManager::LoadDirectoryAsync(const std::string& searchMask) {
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

std::shared_ptr<std::vector<std::shared_ptr<Resource>>> ResourceManager::LoadDirectory(const std::string& searchMask) {
    auto futureList = LoadDirectoryAsync(searchMask);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<Resource>>>();

    for (size_t i = 0; i < futureList->size(); i++) {
        const auto future = futureList->at(i);
        const auto resource = future.get();
        loadedList->push_back(resource);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::string>> ResourceManager::FindLoadedFiles(const std::string& searchMask) {
    const char* wildCard = searchMask.c_str();
    auto list = std::make_shared<std::vector<std::string>>();

    for (const auto& [key, value] : mResourceCache) {
        if (SFileCheckWildCard(key.c_str(), wildCard)) {
            list->push_back(key);
        }
    }

    return list;
}

void ResourceManager::DirtyDirectory(const std::string& searchMask) {
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

void ResourceManager::UnloadDirectory(const std::string& searchMask) {
    auto list = FindLoadedFiles(searchMask);

    for (const auto& key : *list.get()) {
        UnloadResource(key);
    }
}

std::shared_ptr<Archive> ResourceManager::GetArchive() {
    return mArchive;
}

std::shared_ptr<ResourceLoader> ResourceManager::GetResourceLoader() {
    return mResourceLoader;
}

std::shared_ptr<Window> ResourceManager::GetContext() {
    return mContext;
}

size_t ResourceManager::UnloadResource(const std::string& filePath) {
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

bool ResourceManager::OtrSignatureCheck(const char* fileName) {
    static const char* sOtrSignature = "__OTR__";
    return strncmp(fileName, sOtrSignature, strlen(sOtrSignature)) == 0;
}

} // namespace Ship
