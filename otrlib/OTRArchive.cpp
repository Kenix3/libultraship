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
			spdlog::error("({}) Failed to open file {} from mpq archive {}", GetLastError(), filePath.c_str(), mainPath.c_str());
			return nullptr;
		}

		DWORD dwFileSize = SFileGetFileSize(fileHandle, 0);
		std::shared_ptr<char[]> fileData(new char[dwFileSize]);
		DWORD dwBytes;

		if (!SFileReadFile(fileHandle, fileData.get(), dwFileSize, &dwBytes, NULL)) {
			spdlog::error("({}) Failed to read file {} from mpq archive {}", GetLastError(), filePath.c_str(), mainPath.c_str());
			if (!SFileCloseFile(fileHandle)) {
				spdlog::error("({}) Failed to close file {} from mpq after read failure in archive {}", GetLastError(), filePath.c_str(), mainPath.c_str());
			}
			return nullptr;
		}

		if (!SFileCloseFile(fileHandle)) {
			spdlog::error("({}) Failed to close file {} from mpq archive {}", GetLastError(), filePath.c_str(), mainPath.c_str());
		}

		auto file = std::make_shared<OTRFile>();
		file.get()->buffer = fileData;
		file.get()->dwBufferSize = dwFileSize;
		file.get()->parent = std::make_shared<OTRArchive>(*this);
		file.get()->path = filePath;

		return file;
	}

	bool OTRArchive::AddFile(std::string path, uintptr_t fileData, DWORD dwFileSize) {
		HANDLE hFile;

		SYSTEMTIME sysTime;
		GetSystemTime(&sysTime);
		FILETIME t;
		SystemTimeToFileTime(&sysTime, &t);
		ULONGLONG stupidHack = static_cast<uint64_t>(t.dwHighDateTime) << (sizeof(t.dwHighDateTime) * 8) | t.dwLowDateTime;

		if (!SFileCreateFile(mainMPQ, path.c_str(), stupidHack, dwFileSize, 0, MPQ_FILE_COMPRESS, &hFile)) {
			spdlog::error("({}) Failed to create file of {} bytes {} in archive {}", GetLastError(), dwFileSize, path.c_str(), mainPath.c_str());
		}


		if (!SFileWriteFile(hFile, (void*)fileData, dwFileSize, MPQ_COMPRESSION_ZLIB)) {
			spdlog::error("({}) Failed to write {} bytes to {} in archive {}", GetLastError(), dwFileSize, path.c_str(), mainPath.c_str());
			if (!SFileCloseFile(hFile)) {
				spdlog::error("({}) Failed to close file {} after write failure in archive {}", GetLastError(), path.c_str(), mainPath.c_str());
			}
			return false;
		}

		if (!SFileFinishFile(hFile)) {
			spdlog::error("({}) Failed to finish file {} in archive {}", GetLastError(), path.c_str(), mainPath.c_str());
			if (!SFileCloseFile(hFile)) {
				spdlog::error("({}) Failed to close file {} after finish failure in archive {}", GetLastError(), path.c_str(), mainPath.c_str());
			}
			return false;
		}
		// SFileFinishFile already frees the handle, so no need to close it again.

		return true;
	}

	bool OTRArchive::RemoveFile(std::string path) {
		if (!SFileRemoveFile(mainMPQ, path.c_str(), 0)) {
			spdlog::error("({}) Failed to remove file {} in archive {}", GetLastError(), path.c_str(), mainPath.c_str());
			return false;
		}
		
		return true;
	}

	bool OTRArchive::RenameFile(std::string oldPath, std::string newPath) {
		if (!SFileRenameFile(mainMPQ, oldPath.c_str(), newPath.c_str())) {
			spdlog::error("({}) Failed to rename file {} to {} in archive {}", GetLastError(), oldPath.c_str(), newPath.c_str(), mainPath.c_str());
			return false;
		}

		return true;
	}

	std::shared_ptr<std::vector<SFILE_FIND_DATA>> OTRArchive::ListFiles(std::string searchMask) {
		auto fileList = std::make_shared<std::vector<SFILE_FIND_DATA>>();
		SFILE_FIND_DATA findContext;
		HANDLE hFind;

		hFind = SFileFindFirstFile(mainMPQ, searchMask.c_str(), &findContext, nullptr);
		if (hFind && GetLastError() != ERROR_NO_MORE_FILES) {
			fileList.get()->push_back(findContext);

			bool fileFound;
			do {
				fileFound = SFileFindNextFile(hFind, &findContext);

				if (fileFound) {
					fileList.get()->push_back(findContext);
				}
				else if (!fileFound && GetLastError() != ERROR_NO_MORE_FILES) {
					spdlog::error("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
					if (!SListFileFindClose(hFind)) {
						spdlog::error("({}) Failed to close file search {} after failure in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
					}
					return nullptr;
				}
			} while (fileFound);
		}
		else {
			spdlog::error("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
			return nullptr;
		}

		if (!SListFileFindClose(hFind)) {
			spdlog::error("({}) Failed to close file search {} in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
		}

		return fileList;
	}

	bool OTRArchive::Load() {
		return LoadMainMPQ() && LoadPatchMPQs();
	}

	bool OTRArchive::Unload() {
		bool success = true;
		for (auto mpqHandle : mpqHandles) {
			if (!SFileCloseArchive(mpqHandle.second)) {
				spdlog::error("({}) Failed to close mpq {}", GetLastError(), mpqHandle.first.c_str());
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
			spdlog::error("({}) Failed to open main mpq file {}.", GetLastError(), mainPath.c_str());
			return false;
		}

		mpqHandles[mainPath] = mpqHandle;
		mainMPQ = mpqHandle;

		return true;
	}

	bool OTRArchive::LoadPatchMPQ(std::string path) {
		HANDLE patchHandle = NULL;

		if (!SFileOpenArchive((TCHAR*)path.c_str(), 0, MPQ_OPEN_READ_ONLY, &patchHandle)) {
			spdlog::error("({}) Failed to open patch mpq file {} while applying to {}.", GetLastError(), path.c_str(), mainPath.c_str());
			return false;
		}

		if (!SFileOpenPatchArchive(mainMPQ, (TCHAR*)path.c_str(), "", 0)) {
			spdlog::error("({}) Failed to apply patch mpq file {} to main mpq {}.", GetLastError(), path.c_str(), mainPath.c_str());
			return false;
		}

		mpqHandles[path] = patchHandle;

		return true;
	}
}