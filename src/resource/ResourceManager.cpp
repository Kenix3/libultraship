#include "ResourceManager.h"
#include <spdlog/spdlog.h>
#include "resource/File.h"
#include "resource/archive/Archive.h"
#include <algorithm>
#include <thread>
#include "utils/StringHelper.h"
#include "utils/glob.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace Ship {

ResourceManager::ResourceManager() {
}

void ResourceManager::Init(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
                           int32_t reservedThreadCount) {
    mResourceLoader = std::make_shared<ResourceLoader>();
    mArchiveManager = std::make_shared<ArchiveManager>();
    GetArchiveManager()->Init(otrFiles, validHashes);

    // the extra `- 1` is because we reserve an extra thread for spdlog
    size_t threadCount = std::max(1, (int32_t)(std::thread::hardware_concurrency() - reservedThreadCount - 1));
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
    return mArchiveManager != nullptr && mArchiveManager->IsArchiveLoaded();
}

std::shared_ptr<File> ResourceManager::LoadFileProcess(const std::string& filePath,
                                                       std::shared_ptr<ResourceInitData> initData) {
    auto file = mArchiveManager->LoadFile(filePath, initData);
    if (file != nullptr) {
        SPDLOG_TRACE("Loaded File {} on ResourceManager", file->InitData->Path);
    } else {
        SPDLOG_TRACE("Could not load File {} in ResourceManager", filePath);
    }
    return file;
}

std::shared_ptr<IResource> ResourceManager::LoadResourceProcess(const std::string& filePath, uintptr_t owner,
                                                                bool loadExact,
                                                                std::shared_ptr<ResourceInitData> initData) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        const auto newFilePath = filePath.substr(7);
        return LoadResourceProcess(newFilePath, owner, false, initData);
    }

    // Attempt to load the alternate version of the asset, if we fail then we continue trying to load the standard
    // asset.
    if (!loadExact && mAltAssetsEnabled && !filePath.starts_with(IResource::gAltAssetPrefix)) {
        const auto altPath = IResource::gAltAssetPrefix + filePath;
        auto altResource = LoadResourceProcess(altPath, owner, loadExact, initData);

        if (altResource != nullptr) {
            return altResource;
        }
    }

    // While waiting in the queue, another thread could have loaded the resource.
    // In a last attempt to avoid doing work that will be discarded, let's check if the cached version exists.
    auto cacheLine = CheckCache(filePath, owner, loadExact);
    auto cachedResource = GetCachedResource(cacheLine);
    if (cachedResource != nullptr) {
        return cachedResource;
    }

    // Check for resource load errors which can indicate an alternate asset.
    // If we are attempting to load an alternate asset, we can return null
    if (!loadExact && mAltAssetsEnabled && filePath.starts_with(IResource::gAltAssetPrefix)) {
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
    auto file = LoadFileProcess(filePath, initData);
    if (file == nullptr) {
        SPDLOG_TRACE("Failed to load resource file at path {}", filePath);
    }

    // Transform the raw data into a resource
    auto resource = GetResourceLoader()->LoadResource(file);

    // Another thread could have loaded the resource while we were processing, so we want to check before setting to
    // the cache.
    cachedResource = GetCachedResource(filePath, owner, true);

    {
        const std::lock_guard<std::mutex> lock(mMutex);

        if (cachedResource != nullptr) {
            // If another thread has already loaded this resource, discard the work we already did and return from
            // cache.
            resource = cachedResource;
        }

        // Set the cache to the loaded resource
        if (resource != nullptr) {
            mResourceCache[owner][filePath] = resource;
        } else {
            mResourceCache[owner][filePath] = ResourceLoadError::NotFound;
        }
    }

    if (resource != nullptr) {
        SPDLOG_TRACE("Loaded Resource {} on ResourceManager", filePath);
    } else {
        SPDLOG_TRACE("Resource load FAILED {} on ResourceManager", filePath);
    }

    return resource;
}

std::shared_future<std::shared_ptr<IResource>>
ResourceManager::LoadResourceAsync(const std::string& filePath, uintptr_t owner, bool loadExact,
                                   BS::priority_t priority, std::shared_ptr<ResourceInitData> initData) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        auto newFilePath = filePath.substr(7);
        return LoadResourceAsync(newFilePath, owner, loadExact, priority);
    }

    // Check the cache before queueing the job.
    auto cacheCheck = GetCachedResource(filePath, owner, loadExact);
    if (cacheCheck) {
        auto promise = std::make_shared<std::promise<std::shared_ptr<IResource>>>();
        promise->set_value(cacheCheck);
        return promise->get_future().share();
    }

    const auto newFilePath = std::string(filePath);

    return mThreadPool->submit_task(
        std::bind(&ResourceManager::LoadResourceProcess, this, newFilePath, owner, loadExact, initData), priority);
}

std::shared_ptr<IResource> ResourceManager::LoadResource(const std::string& filePath, uintptr_t owner, bool loadExact,
                                                         std::shared_ptr<ResourceInitData> initData) {
    auto resource = LoadResourceAsync(filePath, owner, loadExact, BS::pr::highest, initData).get();
    if (resource == nullptr) {
        SPDLOG_ERROR("Failed to load resource file at path {}", filePath);
    }
    return resource;
}

std::variant<ResourceManager::ResourceLoadError, std::shared_ptr<IResource>>
ResourceManager::CheckCache(const std::string& filePath, uintptr_t owner, bool loadExact) {
    if (!loadExact && mAltAssetsEnabled && !filePath.starts_with(IResource::gAltAssetPrefix)) {
        const auto altPath = IResource::gAltAssetPrefix + filePath;
        auto altCacheResult = CheckCache(altPath, owner, loadExact);

        // If the type held at this cache index is a resource, then we return it.
        // Else we attempt to load standard definition assets.
        if (std::holds_alternative<std::shared_ptr<IResource>>(altCacheResult)) {
            return altCacheResult;
        }
    }

    const std::lock_guard<std::mutex> lock(mMutex);

    auto cacheInstanceFind = mResourceCache.find(owner);
    if (cacheInstanceFind == mResourceCache.end()) {
        return ResourceLoadError::NotCached;
    }

    auto resourceCacheFind = cacheInstanceFind->second.find(filePath);
    if (resourceCacheFind == cacheInstanceFind->second.end()) {
        return ResourceLoadError::NotCached;
    }

    return resourceCacheFind->second;
}

std::shared_ptr<IResource> ResourceManager::GetCachedResource(const std::string& filePath, uintptr_t owner,
                                                              bool loadExact) {
    // Gets the cached resource based on filePath.
    return GetCachedResource(CheckCache(filePath, owner, loadExact));
}

std::shared_ptr<IResource>
ResourceManager::GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<IResource>> cacheLine) {
    // Gets the cached resource based on a cache line std::variant from the cache map.
    if (std::holds_alternative<std::shared_ptr<IResource>>(cacheLine)) {
        try {
            auto resource = std::get<std::shared_ptr<IResource>>(cacheLine);

            if (resource.use_count() <= 0) {
                return nullptr;
            }

            if (resource->IsDirty()) {
                return nullptr;
            }

            return resource;
        } catch (std::bad_variant_access const& e) {
            // Ignore the exception
        }
    }

    return nullptr;
}

std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<IResource>>>>
ResourceManager::LoadDirectoryAsync(const std::string& searchMask, uintptr_t owner, BS::priority_t priority) {
    auto loadedList = std::make_shared<std::vector<std::shared_future<std::shared_ptr<IResource>>>>();
    auto fileList = GetArchiveManager()->ListFiles(searchMask);
    loadedList->reserve(fileList->size());

    for (size_t i = 0; i < fileList->size(); i++) {
        auto fileName = std::string(fileList->operator[](i));
        auto future = LoadResourceAsync(fileName, owner, false, priority);
        loadedList->push_back(future);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::shared_ptr<IResource>>> ResourceManager::LoadDirectory(const std::string& searchMask,
                                                                                        uintptr_t owner) {
    auto futureList = LoadDirectoryAsync(searchMask, owner, true);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<IResource>>>();

    for (size_t i = 0; i < futureList->size(); i++) {
        const auto future = futureList->at(i);
        const auto resource = future.get();
        loadedList->push_back(resource);
    }

    return loadedList;
}

void ResourceManager::DirtyDirectory(const std::string& searchMask, uintptr_t owner) {
    auto list = GetArchiveManager()->ListFiles(searchMask);

    for (const auto& key : *list.get()) {
        auto resource = GetCachedResource(key, owner);
        // If it's a resource, we will set the dirty flag, else we will just unload it.
        if (resource != nullptr) {
            resource->Dirty();
        } else {
            UnloadResource(key, owner);
        }
    }
}

void ResourceManager::UnloadDirectory(const std::string& searchMask, uintptr_t owner) {
    auto list = GetArchiveManager()->ListFiles(searchMask);

    for (const auto& key : *list.get()) {
        UnloadResource(key, owner);
    }
}

std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() {
    return mArchiveManager;
}

std::shared_ptr<ResourceLoader> ResourceManager::GetResourceLoader() {
    return mResourceLoader;
}

size_t ResourceManager::UnloadResource(const std::string& filePath, uintptr_t owner) {
    // Store a shared pointer here so that erase doesn't destruct the resource.
    // The resource will attempt to load other resources on the destructor, and this will fail because we already hold
    // the mutex.
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> value = nullptr;
    size_t ret = 0;
    // We can only erase the resource if we have any resources for that owner.
    if (mResourceCache.contains(owner)) {
        // Need to check if we have the resource within the owner so we don't default construct.
        if (mResourceCache[owner].contains(filePath)) {
            const std::lock_guard<std::mutex> lock(mMutex);

            // Hold the shared pointer for the cache value. Explained above.
            value = mResourceCache[owner][filePath];
            // Remove from the owner's cache map.
            ret = mResourceCache[owner].erase(filePath);

            // If the entire cache is empty, kill the map for that owner.
            if (mResourceCache[owner].empty()) {
                mResourceCache.erase(owner);
            }
        }
    }

    return ret;
}

bool ResourceManager::OtrSignatureCheck(const char* fileName) {
    static const char* sOtrSignature = "__OTR__";
    return strncmp(fileName, sOtrSignature, strlen(sOtrSignature)) == 0;
}

bool ResourceManager::IsAltAssetsEnabled() {
    return mAltAssetsEnabled;
}

void ResourceManager::SetAltAssetsEnabled(bool isEnabled) {
    mAltAssetsEnabled = isEnabled;
}

} // namespace Ship
