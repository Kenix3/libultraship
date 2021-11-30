#include "OTRArchive.h"
#include "OTRResource.h"
#include "OTRFile.h"
#include "spdlog/spdlog.h"
#include "Utils/StringHelper.h"
#include "Lib/StrHash64.h"
#include <filesystem>

namespace OtrLib {
	OTRArchive::OTRArchive(std::string mainPath) : OTRArchive(mainPath, "")
	{
	}

	OTRArchive::OTRArchive(std::string mainPath, std::string patchesDirectory) : mainPath(mainPath), patchesDirectory(patchesDirectory) {
		Load();
	}

	OTRArchive::~OTRArchive() {
		Unload();
	}

	OTRArchive::OTRArchive()
	{
	}

	std::shared_ptr<OTRArchive> OTRArchive::CreateArchive(std::string archivePath)
	{
		OTRArchive* otrArchive = new OTRArchive();
		otrArchive->mainPath = archivePath;

		TCHAR* t_filename = new TCHAR[archivePath.size() + 1];
		t_filename[archivePath.size()] = 0;
		std::copy(archivePath.begin(), archivePath.end(), t_filename);

		bool success = SFileCreateArchive(t_filename, MPQ_CREATE_LISTFILE | MPQ_CREATE_ATTRIBUTES | MPQ_CREATE_ARCHIVE_V2, 65536 * 4, &otrArchive->mainMPQ);
		int error = GetLastError();

		if (success)
		{
			otrArchive->mpqHandles[archivePath] = otrArchive->mainMPQ;
			return std::make_shared<OTRArchive>(*otrArchive);
		}
		else
		{
			spdlog::error("({}) We tried to create an archive, but it has fallen and cannot get up.");
			return nullptr;
		}
	}

	std::shared_ptr<OTRFile> OTRArchive::LoadFile(std::string filePath, bool includeParent) {
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

		if (includeParent)
			file.get()->parent = shared_from_this();
		else
			file.get()->parent = nullptr;

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

		addedFiles.push_back(path);
		hashes[CRC64(path.c_str())] = path;

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

	std::vector<SFILE_FIND_DATA> OTRArchive::ListFiles(std::string searchMask) {
		auto fileList = std::vector<SFILE_FIND_DATA>();
		SFILE_FIND_DATA findContext;
		HANDLE hFind;

		
		hFind = SFileFindFirstFile(mainMPQ, searchMask.c_str(), &findContext, nullptr);
		//if (hFind && GetLastError() != ERROR_NO_MORE_FILES) {
		if (hFind != nullptr) {
			fileList.push_back(findContext);

			bool fileFound;
			do {
				fileFound = SFileFindNextFile(hFind, &findContext);

				if (fileFound) {
					fileList.push_back(findContext);
				}
				else if (!fileFound && GetLastError() != ERROR_NO_MORE_FILES) {
					spdlog::error("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
					if (!SListFileFindClose(hFind)) {
						spdlog::error("({}) Failed to close file search {} after failure in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
					}
					return fileList;
				}
			} while (fileFound);
		}
		else if (GetLastError() != ERROR_NO_MORE_FILES) {
			spdlog::error("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
			return fileList;
		}

		if (!SFileFindClose(hFind)) {
			spdlog::error("({}) Failed to close file search {} in archive {}", GetLastError(), searchMask.c_str(), mainPath.c_str());
		}

		return fileList;
	}

	bool OTRArchive::HasFile(std::string filename)
	{
		auto lst = ListFiles(filename);
		//auto lst = addedFiles;
		
		for (auto item : lst)
		{
			if (item.cFileName == filename)
			{
				return true;
			}
		}

		return false;
	}

	bool OTRArchive::Load() {
		return LoadMainMPQ() && LoadPatchMPQs();
	}

	bool OTRArchive::Unload() 
	{
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

		TCHAR* t_filename = new TCHAR[mainPath.size() + 1];
		t_filename[mainPath.size()] = 0;
		std::copy(mainPath.begin(), mainPath.end(), t_filename);

		if (!SFileOpenArchive(t_filename, 0, 0 /*MPQ_OPEN_READ_ONLY*/, &mpqHandle)) {
			spdlog::error("({}) Failed to open main mpq file {}.", GetLastError(), mainPath.c_str());
			return false;
		}

		mpqHandles[mainPath] = mpqHandle;
		mainMPQ = mpqHandle;

		auto listFile = LoadFile("(listfile)", false);

		std::vector<std::string> lines = StringHelper::Split(std::string(listFile->buffer.get(), listFile->dwBufferSize), "\n");

		for (int i = 0; i < lines.size(); i++)
		{
			uint64_t hash = StringHelper::StrToL(lines[i], 16);
			hashes[hash] = lines[i];
		}

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