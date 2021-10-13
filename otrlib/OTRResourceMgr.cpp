#include "OTRResourceMgr.h"
#include "Factories/OTRResourceLoader.h"
#include <Utils/MemoryStream.h>
#include "spdlog/spdlog.h"
#include "OTRFile.h"
#include "OTRArchive.h"

namespace OtrLib {

	OTRResourceMgr::OTRResourceMgr(std::string mainPath, std::string patchesDirectory) {
		archive = std::make_shared<OTRArchive>(mainPath, patchesDirectory);
	}

	OTRResourceMgr::~OTRResourceMgr() {
		fileCache.clear();
		otrCache.clear();
		gameResourceAddresses.clear();
	}

	std::shared_ptr<OTRFile> OTRResourceMgr::LoadFileFromCache(std::string filePath) {
		// File already loaded...?
		if (fileCache.find(filePath) != fileCache.end()) {
			return fileCache[filePath];
		}
		else {
			spdlog::debug("Cache miss on file load: {}", filePath.c_str());

			auto file = archive.get()->LoadFile(filePath);

			fileCache[filePath] = file;
			return file;
		}
	}

	DWORD OTRResourceMgr::LoadFile(uintptr_t destination, DWORD destinationSize, std::string filePath) {
		std::shared_ptr<OTRFile> fileData = LoadFileFromCache(filePath);

		DWORD copySize = destinationSize >= fileData.get()->dwBufferSize ? fileData.get()->dwBufferSize : destinationSize;
		memcpy((void*)destination, fileData.get()->buffer.get(), copySize);

		if (gameResourceAddresses.find(filePath) == gameResourceAddresses.end()) {
			gameResourceAddresses[filePath] = std::make_shared<std::unordered_set<uintptr_t>>();
		}

		gameResourceAddresses[filePath].get()->insert(destination);

		return copySize;
	}

	void OTRResourceMgr::MarkFileAsFree(uintptr_t destination, DWORD destinationSize, std::string filePath) {
		if (gameResourceAddresses.find(filePath) != gameResourceAddresses.end()) {
			gameResourceAddresses[filePath].get()->erase(destination);
		}
	}

	std::shared_ptr<OTRResource> OTRResourceMgr::LoadOTRFile(std::string filePath) {
		std::shared_ptr<OTRFile> fileData = LoadFileFromCache(filePath);
		std::shared_ptr<OTRResource> resource;

		if (otrCache.find(filePath) != otrCache.end()) {
			resource = otrCache[filePath];

			if (!resource.get()->isDirty) {
				return resource;
			}
			else {
				otrCache.erase(filePath);
			}
		}

		MemoryStream memStream = MemoryStream(fileData.get()->buffer.get(), fileData.get()->dwBufferSize);
		BinaryReader reader = BinaryReader(&memStream);
		resource = std::make_shared<OTRResource>(*OTRResourceLoader::LoadResource(&reader));

		if (resource != nullptr)
			otrCache[filePath] = resource;

		return resource;
	}

	void OTRResourceMgr::CacheDirectory() {
		// TODO: Figure out how "searching" works with StormLib...
	}
}