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

			std::shared_ptr<File> ToLoad = ResourceLoadQueue.front();

			while (!ToLoad->bIsLoaded && !ToLoad->bHasLoadError) {
				std::unique_lock<std::mutex> Lock(ToLoad->FileLoadMutex);
				ToLoad->FileLoadNotifier.wait(Lock);
			}

			SPDLOG_INFO("Loading Resource {} on ResourceMgr thread", ToLoad->path);
			auto unmanagedResource = ResourceLoader::LoadResource(ToLoad);
			auto resource = std::shared_ptr<Resource>(unmanagedResource);

			ResourceCache[resource->File->path] = resource;
			ResourceLoadQueue.pop();

			{
				const std::lock_guard<std::mutex> ResGuard(ResourceLoadMutex);
				resource->File->bHasResourceLoaded = true;
			}


			ToLoad->ResourceLoadNotifier.notify_all();

			SPDLOG_INFO("LOADED Resource {} on ResourceMgr thread", ToLoad->path);

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
				while (!ToLoad->bIsLoaded && !ToLoad->bHasLoadError) {
					std::unique_lock<std::mutex> Lock(ToLoad->FileLoadMutex);
					ToLoad->FileLoadNotifier.wait(Lock);
				}
			} else {
				return ToLoad;
			}
		}

		return FileCache[FilePath];
	}

	std::shared_ptr<Resource> ResourceMgr::LoadResource(std::string FilePath, bool Blocks) {
		FilePath = StringHelper::Replace(FilePath, "/", "\\");

		if (StringHelper::StartsWith(FilePath, "__OTR__"))
			FilePath = StringHelper::Split(FilePath, "__OTR__")[1];

		std::shared_ptr<File> FileData = LoadFile(FilePath, Blocks);

		if (ResourceCache.find(FilePath) == ResourceCache.end() || ResourceCache[FilePath]->isDirty) {
			if (ResourceCache.find(FilePath) == ResourceCache.end()) {
				SPDLOG_DEBUG("Cache miss on Resource load: {}", filePath.c_str());
			}

			{
				const std::lock_guard<std::mutex> ResLock(ResourceLoadMutex);
				FileData->bHasResourceLoaded = false;
				ResourceLoadQueue.push(FileData);
			}
			ResourceLoadNotifier.notify_all();

			if (Blocks) {
				while(!FileData->bHasResourceLoaded) {
					std::unique_lock<std::mutex> Lock(FileData->ResourceLoadMutex);
					FileData->ResourceLoadNotifier.wait(Lock);
				}
			}
			// TODO: We should return an unloaded resource here.
		}

		return ResourceCache[FilePath];
	}

	void ResourceMgr::CacheDirectory(std::string SearchMask, bool Blocks) {
		auto fileList = OTR->ListFiles(SearchMask);

		for (DWORD i = 0; i < fileList.size(); i++) {
			auto file = LoadResource(fileList.operator[](i).cFileName, Blocks);

		}
	}

	std::string ResourceMgr::HashToString(uint64_t Hash) {
		return OTR->HashToString(Hash);
	}
}