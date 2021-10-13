#include "OTRArchive.h"
#include "OTRFile.h"
#include "spdlog/spdlog.h"
#include "Utils/StringHelper.h"
#include <filesystem>

namespace OtrLib {
	OTRArchive::OTRArchive(std::string mainPath) {
		OTRArchive(mainPath, "");
	}

	OTRArchive::OTRArchive(std::string mainPath, std::string patchesDirectory) : mainPath(mainPath), patchesDirectory(patchesDirectory) {
		Load();
	}

	OTRArchive::~OTRArchive() {
		Unload();
	}

	std::shared_ptr<OTRFile> OTRArchive::LoadFile(std::string filePath) {
;		HANDLE fileHandle = NULL;

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
		file.get()->parent = std::make_shared<OTRArchive>(*this);
		file.get()->path = filePath;

		return file;
	}

	bool OTRArchive::AddFile(std::string path, char* fileData, DWORD dwFileSize) {
		HANDLE hFile;

		SYSTEMTIME sysTime;
		GetSystemTime(&sysTime);
		FILETIME t;
		SystemTimeToFileTime(&sysTime, &t);
		ULONGLONG stupidHack = static_cast<uint64_t>(t.dwHighDateTime) << (sizeof(t.dwHighDateTime) * 8) | t.dwLowDateTime;

		if (!SFileCreateFile(mainMPQ, path.c_str(), stupidHack, dwFileSize, 0, MPQ_FILE_COMPRESS, &hFile)) {
			spdlog::error("Failed to create file of {} bytes {} in archive {}", dwFileSize, path.c_str(), mainPath.c_str());
		}


		if (!SFileWriteFile(hFile, fileData, dwFileSize, MPQ_COMPRESSION_ZLIB)) {
			spdlog::error("Failed to write {} bytes to {} in archive {}", dwFileSize, path.c_str(), mainPath.c_str());
			SFileCloseFile(hFile);
			return false;
		}

		if (!SFileFinishFile(hFile)) {
			spdlog::error("Failed to finish file {} in archive {}", dwFileSize, path.c_str(), mainPath.c_str());
			SFileCloseFile(hFile);
			return false;
		}

		return true;
	}

	bool OTRArchive::Load() {
		return LoadMainMPQ() && LoadPatchMPQs();
	}

	bool OTRArchive::Unload() {
		bool success = true;
		for (auto mpqHandle : mpqHandles) {
			if (!SFileCloseArchive(mpqHandle.second)) {
				spdlog::error("Failed to close mpq {}, ERROR CODE: {}", mpqHandle.first.c_str(), GetLastError());
				success = false;
			}
		}

		mainMPQ = nullptr;

		return success;
	}

	bool OTRArchive::LoadPatchMPQs() {
		// TODO: We also want to periodically scan the patch directories for new MPQs. When new MPQs are found we will load the contents to fileCache and then copy over to gameResourceAddresses
		if (patchesDirectory.length() > 0) {
			for (auto& p : std::filesystem::recursive_directory_iterator(patchesDirectory)) {
				if (StringHelper::IEquals(p.path().extension().string(), ".otr") || StringHelper::IEquals(p.path().extension().string(), ".mpq")) {
					if (!LoadPatchMPQ(p.path().string())) {
						return false;
					}
				}
			}
		}

		return true;
	}

	bool OTRArchive::LoadMainMPQ() {
		HANDLE mpqHandle = NULL;

		if (!SFileOpenArchive((TCHAR*)mainPath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle)) {
			spdlog::error("Failed to open main mpq file {}. ERROR CODE: {}", mainPath.c_str(), GetLastError());
			return false;
		}

		mpqHandles[mainPath] = mpqHandle;
		mainMPQ = mpqHandle;

		return true;
	}

	bool OTRArchive::LoadPatchMPQ(std::string path) {
		HANDLE patchHandle = NULL;

		if (!SFileOpenArchive((TCHAR*)path.c_str(), 0, MPQ_OPEN_READ_ONLY, &patchHandle)) {
			spdlog::error("Failed to open patch mpq file {}. ERROR CODE: {}", path.c_str(), GetLastError());
			return false;
		}

		if (!SFileOpenPatchArchive(mainMPQ, (TCHAR*)path.c_str(), "", 0)) {
			spdlog::error("Failed to apply patch mpq file {}. ERROR CODE: {}", path.c_str(), GetLastError());
			return false;
		}

		mpqHandles[path] = patchHandle;

		return true;
	}
}