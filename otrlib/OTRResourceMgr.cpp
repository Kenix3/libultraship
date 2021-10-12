#include "OTRResourceMgr.h"
#include "Factories/OTRResourceLoader.h"
#include <Utils/MemoryStream.h>
#include "spdlog/spdlog.h"
#include "OTRFile.h"
#include "Utils/StringHelper.h"
#include <filesystem>

namespace OtrLib {

	OTRResourceMgr::OTRResourceMgr()
	{
		mainMPQ = nullptr;
	}

	OTRResourceMgr::~OTRResourceMgr() {
		for (auto mpqHandle : mpqHandles) {
			if (!SFileCloseArchive(mpqHandle.second)) {
				spdlog::error("Failed to close mpq {}, ERROR CODE: {}", mpqHandle.first.c_str(), GetLastError());
			}
		}

		mainMPQ = nullptr;
		fileCache.clear();
		otrCache.clear();
		gameResourceAddresses.clear();
	}

	void OTRResourceMgr::LoadPatchArchives(std::string patchesPath) {
		// TODO: We also want the manager to periodically scan the patch directories for new MPQs. When new MPQs are found we will load the contents to fileCache and then copy over to gameResourceAddresses
		if (patchesPath.length() > 0) {
			for (auto& p : std::filesystem::recursive_directory_iterator(patchesPath)) {
				if (StringHelper::IEquals(p.path().extension().string(), ".otr") || StringHelper::IEquals(p.path().extension().string(), ".mpq")) {
					LoadMPQPatch(p.path().string());
				}
			}
		}
	}

	void OTRResourceMgr::LoadArchiveAndPatches(std::string mainArchiveFilePath, std::string patchesPath) {
		LoadMPQMain(mainArchiveFilePath.c_str());
		LoadPatchArchives(patchesPath);
	}

	std::shared_ptr<OTRFile> OTRResourceMgr::LoadFileFromCache(std::string filePath) {
		// File already loaded...?
		if (fileCache.find(filePath) != fileCache.end()) {
			return fileCache[filePath];
		}
		else {
			HANDLE fileHandle = NULL;

			if (!SFileOpenFileEx(mainMPQ, filePath.c_str(), 0, &fileHandle)) {
				spdlog::error("Failed to load file from mpq, ERROR CODE: {}", GetLastError());
				return nullptr;
			}

			DWORD dwFileSize = SFileGetFileSize(fileHandle, 0);
			std::shared_ptr<char[]> fileData(new char[dwFileSize]);
			DWORD dwBytes;

			if (!SFileReadFile(fileHandle, fileData.get(), dwFileSize, &dwBytes, NULL)) {
				if (!SFileCloseFile(fileHandle)) {
					spdlog::error("Failed to close file from mpq after read failure, ERROR CODE: {}", GetLastError());
				}
				spdlog::error("Failed to read file from mpq, ERROR CODE: {}", GetLastError());
				return nullptr;
			}

			if (!SFileCloseFile(fileHandle)) {
				spdlog::error("Failed to close file from mpq, ERROR CODE: {}", GetLastError());
			}

			auto file = std::make_shared<OTRFile>();
			file.get()->buffer = fileData;
			file.get()->dwBufferSize = dwFileSize;

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

	void OTRResourceMgr::FreeFile(uintptr_t destination, DWORD destinationSize, std::string filePath) {
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

	void OTRResourceMgr::LoadMPQMain(std::string filePath) {
		HANDLE mpqHandle = NULL;

		if (!SFileOpenArchive((TCHAR*)filePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle)) {
			spdlog::error("Failed to open main mpq file {}. ERROR CODE: {}", filePath.c_str(), GetLastError());
			return;
		}

		mpqHandles[filePath] = mpqHandle;
		mainMPQ = mpqHandle;
	}

	void OTRResourceMgr::LoadMPQPatch(std::string patchFilePath) {
		HANDLE mpqHandle2 = NULL;

		if (!SFileOpenArchive((TCHAR*)patchFilePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle2)) {
			spdlog::error("Failed to open patch mpq file {}. ERROR CODE: {}", patchFilePath.c_str(), GetLastError());
			return;
		}

		if (!SFileOpenPatchArchive(mainMPQ, (TCHAR*)patchFilePath.c_str(), "", 0)) {
			spdlog::error("Failed to apply patch mpq file {}. ERROR CODE: {}", patchFilePath.c_str(), GetLastError());
			return;
		}

		mpqHandles[patchFilePath] = mpqHandle2;
	}
}