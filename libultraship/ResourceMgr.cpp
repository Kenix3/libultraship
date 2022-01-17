#include "ResourceMgr.h"
#include "Factories/ResourceLoader.h"
#include "spdlog/spdlog.h"
#include "File.h"
#include "Archive.h"
#include <Utils/StringHelper.h>
#include "Lib/StormLib/StormLib.h"

namespace Ship {

	ResourceMgr::ResourceMgr(std::shared_ptr<GlobalCtx2> Context, std::string MainPath, std::string PatchesPath) : Context(Context), bIsRunning(false), FileLoadThread(nullptr) {
		OTR = std::make_shared<Archive>(MainPath, PatchesPath);

		Start();
	}

	ResourceMgr::~ResourceMgr() {
		SPDLOG_INFO("destruct ResourceMgr");
		Stop();

		FileCache.clear();
		ResourceCache.clear();
	}

	void ResourceMgr::Start() {
		const std::lock_guard<std::mutex> FileLock(FileLoadMutex);
		const std::lock_guard<std::mutex> ResLock(ResourceLoadMutex);
		if (!IsRunning()) {
			bIsRunning = true;
			FileLoadThread = std::make_shared<std::thread>(&ResourceMgr::LoadFileThread, this);
			ResourceLoadThread = std::make_shared<std::thread>(&ResourceMgr::LoadResourceThread, this);
		}
	}

	void ResourceMgr::Stop() {
		if (IsRunning()) {
			{
				const std::lock_guard<std::mutex> FileLock(FileLoadMutex);
				const std::lock_guard<std::mutex> ResLock(ResourceLoadMutex);
				bIsRunning = false;
			}
			
			FileLoadNotifier.notify_all();
			ResourceLoadNotifier.notify_all();
			FileLoadThread->join();
			ResourceLoadThread->join();

			if (!FileLoadQueue.empty()) {
				SPDLOG_DEBUG("Resource manager stopped, but has {} Files left to load.", FileLoadQueue.size());
			}

			if (!ResourceLoadQueue.empty()) {
				SPDLOG_DEBUG("Resource manager stopped, but has {} Resources left to load.", FileLoadQueue.size());
			}
		}
	}

	bool ResourceMgr::IsRunning() {
		return bIsRunning && FileLoadThread != nullptr;
	}

	void ResourceMgr::LoadFileThread() {
		SPDLOG_INFO("Resource Manager LoadFileThread started");

		while (true) {
			std::unique_lock<std::mutex> Lock(FileLoadMutex);

			while (bIsRunning && FileLoadQueue.empty()) {
				FileLoadNotifier.wait(Lock);
			}

			if (!bIsRunning) {
				break;
			}

			std::shared_ptr<File> ToLoad = FileLoadQueue.front();
			SPDLOG_INFO("Loading File {} on ResourceMgr thread", ToLoad->path);
			OTR->LoadFile(ToLoad->path, true, ToLoad);
			FileCache[ToLoad->path] = ToLoad->bIsLoaded && !ToLoad->bHasLoadError ? ToLoad : nullptr;

			FileLoadQueue.pop();
			ToLoad->FileLoadNotifier.notify_all();
		}

		SPDLOG_INFO("Resource Manager LoadFileThread ended");
	}

	void ResourceMgr::LoadResourceThread() {
		SPDLOG_INFO("Resource Manager LoadResourceThread started");

		while (true) {
			{
				std::unique_lock<std::mutex> ResLock(ResourceLoadMutex);
				while (bIsRunning && ResourceLoadQueue.empty()) {
					ResourceLoadNotifier.wait(ResLock);
				}
			}

			if (!bIsRunning) {
				break;
			}

			std::shared_ptr<ResourcePromise> ToLoad = ResourceLoadQueue.front();

			// Wait for the underlying File to complete loading
			{
				std::unique_lock<std::mutex> FileLock(ToLoad->File->FileLoadMutex);
				while (!ToLoad->File->bIsLoaded && !ToLoad->File->bHasLoadError) {
					ToLoad->File->FileLoadNotifier.wait(FileLock);
				}
			}

			SPDLOG_INFO("Loading Resource {} on ResourceMgr thread", ToLoad->File->path);
			auto UnmanagedRes = ResourceLoader::LoadResource(ToLoad->File);
			auto Res = std::shared_ptr<Resource>(UnmanagedRes);

			ResourceCache[Res->File->path] = Res;
			ResourceLoadQueue.pop();

			{
				const std::lock_guard<std::mutex> ResGuard(ResourceLoadMutex);
				ToLoad->bHasResourceLoaded = true;
			}

			ToLoad->ResourceLoadNotifier.notify_all();

			SPDLOG_INFO("LOADED Resource {} on ResourceMgr thread", ToLoad->File->path);

		}

		SPDLOG_INFO("Resource Manager LoadResourceThread ended");
	}

	std::shared_ptr<File> ResourceMgr::LoadFile(std::string FilePath, bool Blocks) {
		// File NOT already loaded...?
		if (FileCache.find(FilePath) == FileCache.end()) {
			SPDLOG_DEBUG("Cache miss on File load: {}", filePath.c_str());
			std::shared_ptr<File> ToLoad = std::make_shared<File>();
			ToLoad->path = FilePath;

			{
				const std::lock_guard<std::mutex> Lock(FileLoadMutex);
				FileLoadQueue.push(ToLoad);
			}
			FileLoadNotifier.notify_all();

			// Wait for the File to actually be loaded if we are told to block.
			if (Blocks) {
				std::unique_lock<std::mutex> Lock(ToLoad->FileLoadMutex);
				while (!ToLoad->bIsLoaded && !ToLoad->bHasLoadError) {
					ToLoad->FileLoadNotifier.wait(Lock);
				}
			} else {
				return ToLoad;
			}
		}

		return FileCache[FilePath];
	}

	std::shared_ptr<Resource> ResourceMgr::LoadResource(std::string FilePath) {
		auto Promise = LoadResourceAsync(FilePath);

		std::unique_lock<std::mutex> Lock(Promise->ResourceLoadMutex);
		while (!Promise->bHasResourceLoaded) {
			Promise->ResourceLoadNotifier.wait(Lock);
		}

		return Promise->Resource;
	}

	std::shared_ptr<ResourcePromise> ResourceMgr::LoadResourceAsync(std::string FilePath) {
		FilePath = StringHelper::Replace(FilePath, "/", "\\");

		if (StringHelper::StartsWith(FilePath, "__OTR__"))
			FilePath = StringHelper::Split(FilePath, "__OTR__")[1];

		std::shared_ptr<File> FileData = LoadFile(FilePath, false);
		std::shared_ptr<ResourcePromise> Promise = std::make_shared<ResourcePromise>();
		Promise->File = FileData;

		if (ResourceCache.find(FilePath) == ResourceCache.end() || ResourceCache[FilePath]->isDirty/* || !FileData->bIsLoaded*/) {
			if (ResourceCache.find(FilePath) == ResourceCache.end()) {
				SPDLOG_DEBUG("Cache miss on Resource load: {}", filePath.c_str());
			}

			{
				const std::lock_guard<std::mutex> ResLock(ResourceLoadMutex);
				Promise->bHasResourceLoaded = false;
				ResourceLoadQueue.push(Promise);
			}
			ResourceLoadNotifier.notify_all();
		} else {
			Promise->bHasResourceLoaded = true;
			Promise->Resource = ResourceCache[FilePath];
		}

		return Promise;
	}

	std::shared_ptr<std::vector<std::shared_ptr<File>>> ResourceMgr::CacheDirectory(std::string SearchMask, bool Blocks) {
		auto loadedList = std::make_shared<std::vector<std::shared_ptr<File>>>();
		auto fileList = OTR->ListFiles(SearchMask);

		for (DWORD i = 0; i < fileList.size(); i++) {
			auto file = LoadFile(fileList.operator[](i).cFileName, Blocks);
			if (file != nullptr) {
				loadedList->push_back(file);
			}
		}

		return loadedList;
	}

	std::string ResourceMgr::HashToString(uint64_t Hash) {
		return OTR->HashToString(Hash);
	}
}