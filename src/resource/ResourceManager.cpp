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

size_t ResourceIdentifier::GetHash() const {
    return mHash;
}

ResourceIdentifier::ResourceIdentifier(const std::string& path, const uintptr_t owner,
                                       const std::shared_ptr<Archive> parent)
    : Path(path), Owner(owner), Parent(parent) {
    mHash = CalculateHash();
}

bool ResourceIdentifier::operator==(const ResourceIdentifier& rhs) const {
    return Owner == rhs.Owner && Path == rhs.Path && Parent == rhs.Parent;
}

size_t ResourceIdentifier::CalculateHash() {
    size_t hash = std::hash<std::uintptr_t>{}(Owner) ^ std::hash<std::string>{}(Path);
    if (Parent != nullptr) {
        hash ^= std::hash<std::string>{}(Parent->GetPath());
    }
    return hash;
}

size_t ResourceCacheDataHash::operator()(const ResourceIdentifier& rcd) const {
    return rcd.GetHash();
}

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

std::shared_ptr<File> ResourceManager::LoadFileProcess(const ResourceIdentifier& cacheData,
                                                       std::shared_ptr<ResourceInitData> initData) {
    if (cacheData.Parent == nullptr) {
        return LoadFileProcess(cacheData.Path, initData);
    }
    auto archive = cacheData.Parent;
    auto file = archive->LoadFile(cacheData.Path, initData);
    if (file != nullptr) {
        SPDLOG_TRACE("Loaded File {} on ResourceManager", file->InitData->Path);
    } else {
        SPDLOG_TRACE("Could not load File {} in ResourceManager", cacheData.Path);
    }
    return file;
}

std::shared_ptr<IResource> ResourceManager::LoadResourceProcess(const ResourceIdentifier& identifier, bool loadExact,
                                                                std::shared_ptr<ResourceInitData> initData) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(identifier.Path.c_str())) {
        const auto newFilePath = identifier.Path.substr(7);
        return LoadResourceProcess({ newFilePath, identifier.Owner, identifier.Parent }, false, initData);
    }

    // Attempt to load the alternate version of the asset, if we fail then we continue trying to load the standard
    // asset.
    if (!loadExact && mAltAssetsEnabled && !identifier.Path.starts_with(IResource::gAltAssetPrefix)) {
        const auto altPath = IResource::gAltAssetPrefix + identifier.Path;
        auto altResource = LoadResourceProcess({ altPath, identifier.Owner, identifier.Parent }, loadExact, initData);

        if (altResource != nullptr) {
            return altResource;
        }
    }

    // While waiting in the queue, another thread could have loaded the resource.
    // In a last attempt to avoid doing work that will be discarded, let's check if the cached version exists.
    auto cacheLine = CheckCache(identifier, loadExact);
    auto cachedResource = GetCachedResource(cacheLine);
    if (cachedResource != nullptr) {
        return cachedResource;
    }

    // Check for resource load errors which can indicate an alternate asset.
    // If we are attempting to load an alternate asset, we can return null
    if (!loadExact && mAltAssetsEnabled && identifier.Path.starts_with(IResource::gAltAssetPrefix)) {
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
    auto file = LoadFileProcess(identifier.Path, initData);
    if (file == nullptr) {
        SPDLOG_TRACE("Failed to load resource file at path {}", identifier.Path);
    }

    // Transform the raw data into a resource
    auto resource = GetResourceLoader()->LoadResource(file);

    // Another thread could have loaded the resource while we were processing, so we want to check before setting to
    // the cache.
    cachedResource = GetCachedResource(identifier, true);

    {
        const std::lock_guard<std::mutex> lock(mMutex);

        if (cachedResource != nullptr) {
            // If another thread has already loaded this resource, discard the work we already did and return from
            // cache.
            resource = cachedResource;
        }

        // Set the cache to the loaded resource
        if (resource != nullptr) {
            mResourceCache[identifier] = resource;
        } else {
            mResourceCache[identifier] = ResourceLoadError::NotFound;
        }
    }

    if (resource != nullptr) {
        SPDLOG_TRACE("Loaded Resource {} on ResourceManager", identifier.Path);
    } else {
        SPDLOG_TRACE("Resource load FAILED {} on ResourceManager", identifier.Path);
    }

    return resource;
}

std::shared_ptr<IResource> ResourceManager::LoadResourceProcess(const std::string& filePath, bool loadExact,
                                                                std::shared_ptr<ResourceInitData> initData) {
    return LoadResourceProcess({ filePath, mDefaultCacheOwner, mDefaultCacheArchive }, loadExact, initData);
}

std::shared_future<std::shared_ptr<IResource>>
ResourceManager::LoadResourceAsync(const ResourceIdentifier& identifier, bool loadExact, BS::priority_t priority,
                                   std::shared_ptr<ResourceInitData> initData) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(identifier.Path.c_str())) {
        auto newFilePath = identifier.Path.substr(7);
        return LoadResourceAsync({ newFilePath, identifier.Owner, identifier.Parent }, loadExact, priority);
    }

    // Check the cache before queueing the job.
    auto cacheCheck = GetCachedResource(identifier, loadExact);
    if (cacheCheck) {
        auto promise = std::make_shared<std::promise<std::shared_ptr<IResource>>>();
        promise->set_value(cacheCheck);
        return promise->get_future().share();
    }

    return mThreadPool->submit_task(
        [this, identifier, loadExact, initData]() -> std::shared_ptr<IResource> {
            return LoadResourceProcess(identifier, loadExact, initData);
        },
        priority);
}

std::shared_future<std::shared_ptr<IResource>>
ResourceManager::LoadResourceAsync(const std::string& filePath, bool loadExact, BS::priority_t priority,
                                   std::shared_ptr<ResourceInitData> initData) {
    return LoadResourceAsync({ filePath, mDefaultCacheOwner, mDefaultCacheArchive }, loadExact, priority, initData);
}

std::shared_ptr<IResource> ResourceManager::LoadResource(const ResourceIdentifier& identifier, bool loadExact,
                                                         std::shared_ptr<ResourceInitData> initData) {
    auto resource = LoadResourceAsync(identifier, loadExact, BS::pr::highest, initData).get();
    if (resource == nullptr) {
        SPDLOG_TRACE("Failed to load resource file at path {}", identifier.Path);
    }
    return resource;
}

std::shared_ptr<IResource> ResourceManager::LoadResource(const std::string& filePath, bool loadExact,
                                                         std::shared_ptr<ResourceInitData> initData) {
    return LoadResource({ filePath, mDefaultCacheOwner, mDefaultCacheArchive }, loadExact, initData);
}

std::variant<ResourceManager::ResourceLoadError, std::shared_ptr<IResource>>
ResourceManager::CheckCache(const ResourceIdentifier& cacheData, bool loadExact) {
    if (!loadExact && mAltAssetsEnabled && !cacheData.Path.starts_with(IResource::gAltAssetPrefix)) {
        const auto altPath = IResource::gAltAssetPrefix + cacheData.Path;
        auto altCacheResult = CheckCache({ altPath, cacheData.Owner, cacheData.Parent }, loadExact);

        // If the type held at this cache index is a resource, then we return it.
        // Else we attempt to load standard definition assets.
        if (std::holds_alternative<std::shared_ptr<IResource>>(altCacheResult)) {
            return altCacheResult;
        }
    }

    const std::lock_guard<std::mutex> lock(mMutex);

    auto cacheFind = mResourceCache.find(cacheData);
    if (cacheFind == mResourceCache.end()) {
        return ResourceLoadError::NotCached;
    }

    return cacheFind->second;
}

std::variant<ResourceManager::ResourceLoadError, std::shared_ptr<IResource>>
ResourceManager::CheckCache(const std::string& filePath, bool loadExact) {
    return CheckCache({ filePath, mDefaultCacheOwner, mDefaultCacheArchive }, loadExact);
}

std::shared_ptr<IResource> ResourceManager::GetCachedResource(const ResourceIdentifier& identifier, bool loadExact) {
    // Gets the cached resource based on filePath.
    return GetCachedResource(CheckCache(identifier, loadExact));
}

std::shared_ptr<IResource> ResourceManager::GetCachedResource(const std::string& filePath, bool loadExact) {
    // Gets the cached resource based on filePath.
    return GetCachedResource({ filePath, mDefaultCacheOwner, mDefaultCacheArchive }, loadExact);
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
ResourceManager::LoadDirectoryAsync(const ResourceIdentifier& identifier, BS::priority_t priority) {
    auto loadedList = std::make_shared<std::vector<std::shared_future<std::shared_ptr<IResource>>>>();
    auto fileList = GetArchiveManager()->ListFiles(identifier.Path);
    loadedList->reserve(fileList->size());

    for (size_t i = 0; i < fileList->size(); i++) {
        auto fileName = std::string(fileList->operator[](i));
        auto future = LoadResourceAsync({ fileName, identifier.Owner, identifier.Parent }, false, priority);
        loadedList->push_back(future);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<IResource>>>>
ResourceManager::LoadDirectoryAsync(const std::string& searchMask, BS::priority_t priority) {
    return LoadDirectoryAsync({ searchMask, mDefaultCacheOwner, mDefaultCacheArchive }, priority);
}

std::shared_ptr<std::vector<std::shared_ptr<IResource>>>
ResourceManager::LoadDirectory(const ResourceIdentifier& identifier) {
    auto futureList = LoadDirectoryAsync(identifier, true);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<IResource>>>();

    for (size_t i = 0; i < futureList->size(); i++) {
        const auto future = futureList->at(i);
        const auto resource = future.get();
        loadedList->push_back(resource);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::shared_ptr<IResource>>> ResourceManager::LoadDirectory(const std::string& searchMask) {
    return LoadDirectory({ searchMask, mDefaultCacheOwner, mDefaultCacheArchive });
}

void ResourceManager::DirtyDirectory(const ResourceIdentifier& identifier) {
    auto list = GetArchiveManager()->ListFiles(identifier.Path);

    for (const auto& key : *list.get()) {
        auto resource = GetCachedResource({ key, identifier.Owner, identifier.Parent });
        // If it's a resource, we will set the dirty flag, else we will just unload it.
        if (resource != nullptr) {
            resource->Dirty();
        } else {
            UnloadResource(identifier);
        }
    }
}

void ResourceManager::DirtyDirectory(const std::string& searchMask) {
    DirtyDirectory({ searchMask, mDefaultCacheOwner, mDefaultCacheArchive });
}

void ResourceManager::UnloadDirectory(const ResourceIdentifier& identifier) {
    auto list = GetArchiveManager()->ListFiles(identifier.Path);

    for (const auto& key : *list.get()) {
        UnloadResource({ key, identifier.Owner, identifier.Parent });
    }
}

void ResourceManager::UnloadDirectory(const std::string& searchMask) {
    UnloadDirectory({ searchMask, mDefaultCacheOwner, mDefaultCacheArchive });
}

std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() {
    return mArchiveManager;
}

std::shared_ptr<ResourceLoader> ResourceManager::GetResourceLoader() {
    return mResourceLoader;
}

size_t ResourceManager::UnloadResource(const ResourceIdentifier& identifier) {
    // Store a shared pointer here so that erase doesn't destruct the resource.
    // The resource will attempt to load other resources on the destructor, and this will fail because we already hold
    // the mutex.
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> value = nullptr;
    size_t ret = 0;
    // We can only erase the resource if we have any resources for that owner.
    if (mResourceCache.contains(identifier)) {
        const std::lock_guard<std::mutex> lock(mMutex);
        mResourceCache.erase(identifier);
    }

    return ret;
}

size_t ResourceManager::UnloadResource(const std::string& filePath) {
    return UnloadResource({ filePath, mDefaultCacheOwner, mDefaultCacheArchive });
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
