#include "ResourceMgr.h"
#include <spdlog/spdlog.h>
#include "OtrFile.h"
#include "Archive.h"
#include "GameVersions.h"
#include <algorithm>
#include <thread>
#include <Utils/StringHelper.h>
#include <StormLib.h>

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

std::shared_ptr<OtrFile> ResourceMgr::LoadFileProcess(const std::string& fileToLoad) {
    auto file = mArchive->LoadFile(fileToLoad, true);
    SPDLOG_TRACE("Loaded File {} on ResourceMgr thread", file->Path);
    return file;
}

std::shared_ptr<Resource> ResourceMgr::LoadResourceProcess(const std::string& fileToLoad) {
    // While waiting in the queue, another thread could have loaded the resource.
    // In a last attempt to avoid doing work that will be discarded, let's check if the cached version exists.
    auto cacheCheck = GetCachedResource(fileToLoad);
    if (cacheCheck != nullptr) {
        return cacheCheck;
    }

    auto file = LoadFileProcess(fileToLoad);
    auto resource = GetResourceLoader()->LoadResource(file);
    auto cachedResource = GetCachedResource(fileToLoad);

    {
        // Another thread could have loaded the resource while we were processing, so we want to check before setting to the cache.
        const std::lock_guard<std::mutex> lock(mMutex);
        if (cachedResource == nullptr) {
            mResourceCache[fileToLoad] = resource;
        } else {
            // If another thread has already loaded this resource, discard the work we already did and return from cache.
            resource = cachedResource;
        }
    }

    if (resource != nullptr) {
        SPDLOG_TRACE("Loaded Resource {} on ResourceMgr thread", fileToLoad);
    } else {
        SPDLOG_ERROR("Resource load FAILED {} on ResourceMgr thread", fileToLoad);
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
    if (filePath[0] == '_' && filePath[1] == '_' && filePath[2] == 'O' && filePath[3] == 'T' && filePath[4] == 'R' &&
        filePath[5] == '_' && filePath[6] == '_') {
        auto newFilePath = filePath.substr(7);
        return LoadResourceAsync(newFilePath);
    }

    auto cacheCheck = GetCachedResource(filePath);
    if (cacheCheck) {
        auto promise = std::make_shared<std::promise<std::shared_ptr<Resource>>>();
        promise->set_value(cacheCheck);
        return promise->get_future().share();
    }

    const auto newFilePath = std::string(filePath);

    return mThreadPool->submit(&ResourceMgr::LoadResourceProcess, this, newFilePath);
}

std::shared_ptr<Resource> ResourceMgr::LoadResource(const std::string& filePath) {
    return LoadResourceAsync(filePath).get();
}

std::shared_ptr<Resource> ResourceMgr::GetCachedResource(const std::string& filePath) {
    const std::lock_guard<std::mutex> lock(mMutex);

    auto resCacheFind = mResourceCache.find(filePath);

    if (resCacheFind == mResourceCache.end()) {
        return nullptr;
    }

    if (resCacheFind->second.use_count() <= 0) {
        return nullptr;
    }

    if (resCacheFind->second->IsDirty) {
        return nullptr;
    }

    return resCacheFind->second;
}

std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<Resource>>>>
ResourceMgr::CacheDirectoryAsync(const std::string& searchMask) {
    auto loadedList = std::make_shared<std::vector<std::shared_future<std::shared_ptr<Resource>>>>();
    auto fileList = ListFiles(searchMask);
    loadedList->reserve(fileList->size());

    for (size_t i = 0; i < fileList->size(); i++) {
        auto fileName = std::string(fileList->operator[](i));
        auto future = LoadResourceAsync(fileName);
        loadedList->push_back(future);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::shared_ptr<Resource>>> ResourceMgr::CacheDirectory(const std::string& searchMask) {
    auto futureList = CacheDirectoryAsync(searchMask);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<Resource>>>();

    for (size_t i = 0; i < futureList->size(); i++) {
        const auto future = futureList->at(i);
        const auto resource = future.get();
        loadedList->push_back(resource);
    }

    return loadedList;
}

size_t ResourceMgr::DirtyDirectory(const std::string& searchMask) {
    auto fileList = ListFiles(searchMask);
    size_t countDirtied = 0;

    for (size_t i = 0; i < fileList->size(); i++) {
        auto fileName = std::string(fileList->operator[](i));

        // We want to synchronously load the resource here because we don't know if it's in the thread pool queue.
        auto cacheCheck = LoadResource(fileName);
        if (cacheCheck != nullptr) {
            cacheCheck->IsDirty = true;
            countDirtied++;
        }
    }

    return countDirtied;
}

std::shared_ptr<std::vector<std::string>> ResourceMgr::ListFiles(const std::string& searchMask) {
    auto result = std::make_shared<std::vector<std::string>>();
    auto fileList = mArchive->ListFiles(searchMask);

    for (size_t i = 0; i < fileList.size(); i++) {
        result->push_back(fileList[i].cFileName);
    }

    return result;
}

void ResourceMgr::InvalidateResourceCache() {
    mResourceCache.clear();
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

int32_t ResourceMgr::OtrSignatureCheck(char* imgData) {
    uintptr_t i = (uintptr_t)(imgData);

    // if (i == 0xD9000000 || i == 0xE7000000 || (i & 1) == 1)
    if ((i & 1) == 1) {
        return 0;
    }

    // if ((i & 0xFF000000) != 0xAB000000 && (i & 0xFF000000) != 0xCD000000 && i != 0) {
    if (i != 0) {
        if (imgData[0] == '_' && imgData[1] == '_' && imgData[2] == 'O' && imgData[3] == 'T' && imgData[4] == 'R' &&
            imgData[5] == '_' && imgData[6] == '_') {
            return 1;
        }
    }

    return 0;
}

size_t ResourceMgr::UnloadResource(const std::string& filePath) {
    return mResourceCache.erase(filePath);
}

void ResourceMgr::UnloadAllResources() {
    mResourceCache.clear();
}

} // namespace Ship
