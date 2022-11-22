#include "ResourceMgr.h"
#include <spdlog/spdlog.h>
#include "OtrFile.h"
#include "Archive.h"
#include "GameVersions.h"
#include <Utils/StringHelper.h>
#include <StormLib.h>

namespace Ship {

ResourceMgr::ResourceMgr(std::shared_ptr<Window> context, const std::string& mainPath, const std::string& patchesPath,
                         const std::unordered_set<uint32_t>& validHashes)
    : mContext(context), mIsRunning(false), mFileLoadThread(nullptr) {
    mResourceLoader = std::make_shared<ResourceLoader>(context);
    mArchive = std::make_shared<Archive>(mainPath, patchesPath, validHashes, false);

    if (mArchive->IsMainMPQValid()) {
        Start();
    }
}

ResourceMgr::ResourceMgr(std::shared_ptr<Window> context, const std::vector<std::string>& otrFiles,
                         const std::unordered_set<uint32_t>& validHashes)
    : mContext(context), mIsRunning(false), mFileLoadThread(nullptr) {
    mResourceLoader = std::make_shared<ResourceLoader>(context);
    mArchive = std::make_shared<Archive>(otrFiles, validHashes, false);

    if (mArchive->IsMainMPQValid()) {
        Start();
    }
}

ResourceMgr::~ResourceMgr() {
    SPDLOG_INFO("destruct ResourceMgr");
    Stop();

    mFileCache.clear();
    mResourceCache.clear();
}

void ResourceMgr::Start() {
    const std::lock_guard<std::mutex> fileLock(mFileLoadMutex);
    const std::lock_guard<std::mutex> resLock(mResourceLoadMutex);
    if (!IsRunning()) {
        mIsRunning = true;
        mFileLoadThread = std::make_shared<std::thread>(&ResourceMgr::LoadFileThread, this);
        mResourceLoadThread = std::make_shared<std::thread>(&ResourceMgr::LoadResourceThread, this);
    }
}

void ResourceMgr::Stop() {
    if (IsRunning()) {
        {
            const std::lock_guard<std::mutex> fileLock(mFileLoadMutex);
            const std::lock_guard<std::mutex> resLock(mResourceLoadMutex);
            mIsRunning = false;
        }

        mFileLoadNotifier.notify_all();
        mResourceLoadNotifier.notify_all();
        mFileLoadThread->join();
        mResourceLoadThread->join();

        if (!mFileLoadQueue.empty()) {
            SPDLOG_INFO("Resource manager stopped, but has {} Files left to load.", mFileLoadQueue.size());
        }

        if (!mResourceLoadQueue.empty()) {
            SPDLOG_INFO("Resource manager stopped, but has {} Resources left to load.", mFileLoadQueue.size());
        }
    }
}

bool ResourceMgr::IsRunning() {
    return mIsRunning && mFileLoadThread != nullptr;
}

bool ResourceMgr::DidLoadSuccessfully() {
    return mArchive != nullptr && mArchive->IsMainMPQValid();
}

void ResourceMgr::LoadFileThread() {
    SPDLOG_INFO("Resource Manager LoadFileThread started");

    while (true) {
        std::unique_lock<std::mutex> lock(mFileLoadMutex);

        while (mIsRunning && mFileLoadQueue.empty()) {
            mFileLoadNotifier.wait(lock);
        }

        if (!mIsRunning) {
            break;
        }

        std::shared_ptr<OtrFile> toLoad = mFileLoadQueue.front();
        mFileLoadQueue.pop();

        mArchive->LoadFile(toLoad->Path, true, toLoad);

        if (!toLoad->HasLoadError) {
            mFileCache[toLoad->Path] = toLoad->IsLoaded && !toLoad->HasLoadError ? toLoad : nullptr;
        }

        SPDLOG_TRACE("Loaded File {} on ResourceMgr thread", toLoad->Path);

        toLoad->FileLoadNotifier.notify_all();
    }

    SPDLOG_INFO("Resource Manager LoadFileThread ended");
}

void ResourceMgr::LoadResourceThread() {
    SPDLOG_INFO("Resource Manager LoadResourceThread started");

    while (true) {
        std::unique_lock<std::mutex> resLock(mResourceLoadMutex);
        while (mIsRunning && mResourceLoadQueue.empty()) {
            mResourceLoadNotifier.wait(resLock);
        }

        if (!mIsRunning) {
            break;
        }

        std::shared_ptr<ResourcePromise> toLoad = mResourceLoadQueue.front();
        mResourceLoadQueue.pop();

        // Wait for the underlying File to complete loading
        {
            std::unique_lock<std::mutex> fileLock(toLoad->File->FileLoadMutex);
            while (!toLoad->File->IsLoaded && !toLoad->File->HasLoadError) {
                toLoad->File->FileLoadNotifier.wait(fileLock);
            }
        }

        if (!toLoad->File->HasLoadError) {
            std::shared_ptr<Resource> resource = GetResourceLoader()->LoadResource(toLoad->File);
            resource->ResourceManager = GetContext()->GetResourceManager();

            if (resource != nullptr) {
                std::unique_lock<std::mutex> lock(toLoad->ResourceLoadMutex);

                toLoad->HasResourceLoaded = true;
                toLoad->Res = resource;
                mResourceCache[resource->File->Path] = resource;

                SPDLOG_TRACE("Loaded Resource {} on ResourceMgr thread", toLoad->File->Path);

                resource->File = nullptr;
            } else {
                toLoad->HasResourceLoaded = false;
                toLoad->Res = nullptr;

                SPDLOG_ERROR("Resource load FAILED {} on ResourceMgr thread", toLoad->File->Path);
            }
        } else {
            toLoad->HasResourceLoaded = false;
            toLoad->Res = nullptr;
        }

        toLoad->ResourceLoadNotifier.notify_all();
    }

    SPDLOG_INFO("Resource Manager LoadResourceThread ended");
}

std::vector<uint32_t> ResourceMgr::GetGameVersions() {
    return mArchive->GetGameVersions();
}

void ResourceMgr::PushGameVersion(uint32_t newGameVersion) {
    mArchive->PushGameVersion(newGameVersion);
}

std::shared_ptr<OtrFile> ResourceMgr::LoadFileAsync(const std::string& filePath) {
    const std::lock_guard<std::mutex> lock(mFileLoadMutex);
    // File NOT already loaded...?
    auto fileCacheFind = mFileCache.find(filePath);
    if (fileCacheFind == mFileCache.end()) {
        SPDLOG_TRACE("Cache miss on File load: {}", filePath.c_str());
        std::shared_ptr<OtrFile> toLoad = std::make_shared<OtrFile>();
        toLoad->Path = filePath;

        mFileLoadQueue.push(toLoad);
        mFileLoadNotifier.notify_all();

        return toLoad;
    }

    return fileCacheFind->second;
}

std::shared_ptr<OtrFile> ResourceMgr::LoadFile(const std::string& filePath) {
    auto toLoad = LoadFileAsync(filePath);
    // Wait for the File to actually be loaded if we are told to block.
    std::unique_lock<std::mutex> lock(toLoad->FileLoadMutex);
    while (!toLoad->IsLoaded && !toLoad->HasLoadError) {
        toLoad->FileLoadNotifier.wait(lock);
    }

    return toLoad;
}

std::shared_ptr<Ship::Resource> ResourceMgr::GetCachedFile(const char* filePath) const {
    auto resCacheFind = mResourceCache.find(filePath);

    if (resCacheFind != mResourceCache.end() && resCacheFind->second.use_count() > 0) {
        return resCacheFind->second;
    } else {
        return nullptr;
    }
}

std::shared_ptr<Resource> ResourceMgr::LoadResource(const char* filePath) {
    auto resource = LoadResourceAsync(filePath);

    if (std::holds_alternative<std::shared_ptr<Resource>>(resource)) {
        return std::get<std::shared_ptr<Resource>>(resource);
    }

    auto& promise = std::get<std::shared_ptr<ResourcePromise>>(resource);

    if (!promise->HasResourceLoaded) {
        std::unique_lock<std::mutex> lock(promise->ResourceLoadMutex);
        while (!promise->HasResourceLoaded) {
            promise->ResourceLoadNotifier.wait(lock);
        }
    }

    return promise->Res;
}

std::variant<std::shared_ptr<Resource>, std::shared_ptr<ResourcePromise>>
ResourceMgr::LoadResourceAsync(const char* filePath) {
    if (filePath[0] == '_' && filePath[1] == '_' && filePath[2] == 'O' && filePath[3] == 'T' && filePath[4] == 'R' &&
        filePath[5] == '_' && filePath[6] == '_') {
        filePath += 7;
    }

    const std::lock_guard<std::mutex> resLock(mResourceLoadMutex);
    auto resCacheFind = mResourceCache.find(filePath);
    if (resCacheFind == mResourceCache.end() || resCacheFind->second->IsDirty) {
        if (resCacheFind == mResourceCache.end()) {
            SPDLOG_TRACE("Cache miss on Resource load: {}", filePath);
        }

        std::shared_ptr<ResourcePromise> promise = std::make_shared<ResourcePromise>();
        std::shared_ptr<OtrFile> fileData = LoadFile(filePath);
        promise->File = fileData;

        if (promise->File->HasLoadError) {
            promise->HasResourceLoaded = true;
        } else {
            promise->HasResourceLoaded = false;
            mResourceLoadQueue.push(promise);
            mResourceLoadNotifier.notify_all();
        }

        return promise;
    } else {
        return resCacheFind->second;
    }
}

std::shared_ptr<std::vector<std::shared_ptr<ResourcePromise>>>
ResourceMgr::CacheDirectoryAsync(const std::string& searchMask) {
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<ResourcePromise>>>();
    auto fileList = mArchive->ListFiles(searchMask);

    for (DWORD i = 0; i < fileList.size(); i++) {
        auto resource = LoadResourceAsync(fileList.operator[](i).cFileName);
        if (std::holds_alternative<std::shared_ptr<Resource>>(resource)) {
            auto promise = std::make_shared<ResourcePromise>();
            promise->HasResourceLoaded = true;
            promise->Res = std::get<std::shared_ptr<Resource>>(resource);
            resource = promise;
        }
        loadedList->push_back(std::get<std::shared_ptr<ResourcePromise>>(resource));
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::shared_ptr<Resource>>> ResourceMgr::CacheDirectory(const std::string& searchMask) {
    auto promiseList = CacheDirectoryAsync(searchMask);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<Resource>>>();

    for (size_t i = 0; i < promiseList->size(); i++) {
        auto promise = promiseList->at(i);

        std::unique_lock<std::mutex> lock(promise->ResourceLoadMutex);
        while (!promise->HasResourceLoaded) {
            promise->ResourceLoadNotifier.wait(lock);
        }

        loadedList->push_back(promise->Res);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::shared_ptr<Resource>>> ResourceMgr::DirtyDirectory(const std::string& searchMask) {
    auto promiseList = CacheDirectoryAsync(searchMask);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<Resource>>>();

    for (size_t i = 0; i < promiseList->size(); i++) {
        auto promise = promiseList->at(i);

        std::unique_lock<std::mutex> lock(promise->ResourceLoadMutex);
        while (!promise->HasResourceLoaded) {
            promise->ResourceLoadNotifier.wait(lock);
        }

        if (promise->Res != nullptr) {
            promise->Res->IsDirty = true;
        }

        loadedList->push_back(promise->Res);
    }

    return loadedList;
}

std::shared_ptr<std::vector<std::string>> ResourceMgr::ListFiles(std::string searchMask) {
    auto result = std::make_shared<std::vector<std::string>>();
    auto fileList = mArchive->ListFiles(searchMask);

    for (DWORD i = 0; i < fileList.size(); i++) {
        result->push_back(fileList[i].cFileName);
    }

    return result;
}

void ResourceMgr::InvalidateResourceCache() {
    mResourceCache.clear();
}

const std::string* ResourceMgr::HashToString(uint64_t hash) const {
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

std::shared_ptr<Resource> ResourceMgr::LoadResource(const std::string& filePath) {
    return LoadResource(filePath.c_str());
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

} // namespace Ship
