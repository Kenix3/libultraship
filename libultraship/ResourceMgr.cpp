#include "ResourceMgr.h"
#include "Factories/ResourceLoader.h"
#include <Utils/MemoryStream.h>
#include "spdlog/spdlog.h"
#include "File.h"
#include "Archive.h"
#include <Utils/StringHelper.h>
#include "Lib/StormLib/StormLib.h"

namespace Ship {

	ResourceMgr::ResourceMgr(std::shared_ptr<GlobalCtx2> Context, std::string MainPath, std::string PatchesPath) : Context(Context), bIsRunning(false), Thread(nullptr) {
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
		const std::lock_guard<std::mutex> Lock(Mutex);
		if (!IsRunning()) {
			bIsRunning = true;
			Thread = std::make_shared<std::thread>(&ResourceMgr::Run, this);
		}
	}

	void ResourceMgr::Stop() {
		if (IsRunning()) {
			{
				const std::lock_guard<std::mutex> Lock(Mutex);
				bIsRunning = false;
			}
			
			Notifier.notify_all();
			Thread->join();
			if (!FileLoadQueue.empty()) {
				SPDLOG_DEBUG("Resource manager stopped, but has {} items left to load.", FileLoadQueue.size());
			}
		}
	}

	bool ResourceMgr::IsRunning() {
		return bIsRunning && Thread != nullptr;
	}

	void ResourceMgr::Run() {
		SPDLOG_INFO("Resource Manager thread started");

		while (true) {
			std::unique_lock<std::mutex> Lock(Mutex);

			while (bIsRunning && FileLoadQueue.empty()) {
				Notifier.wait(Lock);
			}

			if (!bIsRunning) {
				break;
			}

			std::shared_ptr<File> ToLoad = FileLoadQueue.front();
			SPDLOG_INFO("Loading file {} on ResourceMgr thread", ToLoad->path);
			CacheFileFromArchive(ToLoad);
			FileLoadQueue.pop();
			ToLoad->Notifier.notify_all();
		}

		SPDLOG_INFO("Resource Manager thread ended");
	}

	void ResourceMgr::CacheFileFromArchive(std::shared_ptr<File> ToLoad) {
		SPDLOG_DEBUG("Loading file to cache: {}", filePath.c_str());

		OTR->LoadFile(ToLoad->path, true, ToLoad);
		FileCache[ToLoad->path] = ToLoad;
	}

	std::shared_ptr<File> ResourceMgr::LoadFile(std::string FilePath, bool Blocks) {
		FilePath = StringHelper::Replace(FilePath, "/", "\\");

		if (StringHelper::StartsWith(FilePath, "__OTR__"))
			FilePath = StringHelper::Split(FilePath, "__OTR__")[1];

		// File already loaded...?
		if (FileCache.find(FilePath) == FileCache.end()) {
			SPDLOG_DEBUG("Cache miss on file load: {}", filePath.c_str());
			std::shared_ptr<File> ToLoad = std::make_shared<File>();
			ToLoad->path = FilePath;

			{
				const std::lock_guard<std::mutex> Lock(Mutex);
				FileLoadQueue.push(ToLoad);
			}
			Notifier.notify_all();

			// Wait for the File to actually be loaded if we are told to block.
			if (Blocks) {
				while (!ToLoad->bIsLoaded && !ToLoad->bHasLoadError) {
					std::unique_lock<std::mutex> Lock(ToLoad->Mutex);
					ToLoad->Notifier.wait(Lock);
				}
			}

			return ToLoad;
		}

		return FileCache[FilePath];
	}

	std::string ResourceMgr::HashToString(uint64_t Hash) {
		return OTR->HashToString(Hash);
	}

	std::shared_ptr<Resource> ResourceMgr::LoadResource(std::string FilePath, bool Blocks) {
		std::shared_ptr<File> fileData = LoadFile(FilePath, Blocks);
		std::shared_ptr<Resource> resource;

		if (ResourceCache.find(FilePath) != ResourceCache.end()) {
			resource = ResourceCache[FilePath];

			if (!resource->isDirty) {
				return resource;
			}
			else {
				ResourceCache.erase(FilePath);
			}
		}

		auto memStream = std::make_shared<MemoryStream>(fileData->buffer.get(), fileData->dwBufferSize);
		//MemoryStream memStream = MemoryStream(fileData->buffer.get(), fileData->dwBufferSize);
		BinaryReader reader = BinaryReader(memStream);
		auto unmanagedResource = ResourceLoader::LoadResource(&reader);
		resource = std::shared_ptr<Resource>(unmanagedResource);

		if (resource != nullptr) {
			ResourceCache[FilePath] = resource;
		}

		return resource;
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
}