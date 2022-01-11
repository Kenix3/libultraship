#include "ResourceMgr.h"
#include "Factories/ResourceLoader.h"
#include <Utils/MemoryStream.h>
#include "spdlog/spdlog.h"
#include "File.h"
#include "Archive.h"
#include "Scene.h"
#include <Utils/StringHelper.h>
#include "Lib/StormLib/StormLib.h"

namespace Ship {

	ResourceMgr::ResourceMgr(std::shared_ptr<GlobalCtx2> Context, std::string MainPath, std::string PatchesPath) : Context(Context) {
		archive = std::make_shared<Archive>(MainPath, PatchesPath);
	}

	ResourceMgr::~ResourceMgr() {
		fileCache.clear();
		otrCache.clear();
		gameResourceAddresses.clear();

		SPDLOG_INFO("destruct resourcemgr");
	}

	std::shared_ptr<File> ResourceMgr::LoadFileFromCache(std::string filePath) 
	{
		filePath = StringHelper::Replace(filePath, "/", "\\");

		if (StringHelper::StartsWith(filePath, "__OTR__"))
			filePath = StringHelper::Split(filePath, "__OTR__")[1];

		// File already loaded...?
		if (fileCache.find(filePath) != fileCache.end()) {
			return fileCache[filePath];
		}
		else 
		{
			SPDLOG_DEBUG("Cache miss on file load: {}", filePath.c_str());

			auto file = archive.get()->LoadFile(filePath);

			if (file != nullptr) {
				fileCache[filePath] = file;
			}

			return file;
		}
	}

	char* ResourceMgr::LoadFileOriginal(std::string filePath)
	{
		std::shared_ptr<File> fileData = LoadFileFromCache(filePath);

		return (char*)fileData.get()->buffer.get();
	}

	DWORD ResourceMgr::LoadFile(uintptr_t destination, DWORD destinationSize, std::string filePath) {
		std::shared_ptr<File> fileData = LoadFileFromCache(filePath);

		DWORD copySize = destinationSize >= fileData.get()->dwBufferSize ? fileData.get()->dwBufferSize : destinationSize;
		memcpy((void*)destination, fileData.get()->buffer.get(), copySize);

		if (gameResourceAddresses.find(filePath) == gameResourceAddresses.end()) {
			gameResourceAddresses[filePath] = std::make_shared<std::unordered_set<uintptr_t>>();
		}

		gameResourceAddresses[filePath].get()->insert(destination);

		return copySize;
	}

	void ResourceMgr::MarkFileAsFree(uintptr_t destination, DWORD destinationSize, std::string filePath) {
		if (gameResourceAddresses.find(filePath) != gameResourceAddresses.end()) {
			gameResourceAddresses[filePath].get()->erase(destination);
		}
	}

	std::string ResourceMgr::HashToString(uint64_t hash)
	{
		return archive->HashToString(hash);
	}

	std::shared_ptr<Resource> ResourceMgr::LoadOTRFile(std::string filePath) {
		std::shared_ptr<File> fileData = LoadFileFromCache(filePath);
		std::shared_ptr<Resource> resource;

		if (otrCache.find(filePath) != otrCache.end()) {
			resource = otrCache[filePath];

			if (!resource.get()->isDirty) {
				return resource;
			}
			else {
				otrCache.erase(filePath);
			}
		}


		auto memStream = std::make_shared<MemoryStream>(fileData.get()->buffer.get(), fileData.get()->dwBufferSize);
		//MemoryStream memStream = MemoryStream(fileData.get()->buffer.get(), fileData.get()->dwBufferSize);
		BinaryReader reader = BinaryReader(memStream);
		auto unmanagedResource = ResourceLoader::LoadResource(&reader);
		resource = std::shared_ptr<Resource>(unmanagedResource);

		if (resource != nullptr) {
			otrCache[filePath] = resource;
		}

		return resource;
	}

	std::shared_ptr<std::vector<std::shared_ptr<File>>> ResourceMgr::CacheDirectory(std::string searchMask) {
		auto loadedList = std::make_shared<std::vector<std::shared_ptr<File>>>();
		auto fileList = archive.get()->ListFiles(searchMask);

		for (DWORD i = 0; i < fileList.size(); i++) {
			auto file = LoadFileFromCache(fileList.operator[](i).cFileName);
			if (file != nullptr) {
				loadedList.get()->push_back(file);
			}
		}

		return loadedList;
	}
}