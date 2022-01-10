#include "OTRArchive.h"
#include "OTRResource.h"
#include "OTRFile.h"
#include "spdlog/spdlog.h"
#include "Utils/StringHelper.h"
#include "Lib/StrHash64.h"
#include <filesystem>

namespace OtrLib {
	OTRArchive::OTRArchive(std::string MainPath) : OTRArchive(MainPath, "", false) {
	}

	OTRArchive::OTRArchive(std::string MainPath, std::string PatchesPath, bool genCRCMap) : MainPath(MainPath), PatchesPath(PatchesPath) {
		Load(genCRCMap);
	}

	OTRArchive::~OTRArchive() {
		Unload();
	}

	OTRArchive::OTRArchive() {
	}

	std::shared_ptr<OTRArchive> OTRArchive::CreateArchive(std::string archivePath)
	{
		OTRArchive* otrArchive = new OTRArchive();
		otrArchive->MainPath = archivePath;

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
			SPDLOG_ERROR("({}) We tried to create an archive, but it has fallen and cannot get up.");
			return nullptr;
		}
	}

	std::shared_ptr<OTRFile> OTRArchive::LoadFile(std::string filePath, bool includeParent) {
		HANDLE fileHandle = NULL;

		if (!SFileOpenFileEx(mainMPQ, filePath.c_str(), 0, &fileHandle)) {
			SPDLOG_ERROR("({}) Failed to open file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			return nullptr;
		}

		DWORD dwFileSize = SFileGetFileSize(fileHandle, 0);
		std::shared_ptr<char[]> fileData(new char[dwFileSize]);
		DWORD dwBytes;

		if (!SFileReadFile(fileHandle, fileData.get(), dwFileSize, &dwBytes, NULL)) {
			SPDLOG_ERROR("({}) Failed to read file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			if (!SFileCloseFile(fileHandle)) {
				SPDLOG_ERROR("({}) Failed to close file {} from mpq after read failure in archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			}
			return nullptr;
		}

		if (!SFileCloseFile(fileHandle)) {
			SPDLOG_ERROR("({}) Failed to close file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
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
			SPDLOG_ERROR("({}) Failed to create file of {} bytes {} in archive {}", GetLastError(), dwFileSize, path.c_str(), MainPath.c_str());
			return false;
		}


		if (!SFileWriteFile(hFile, (void*)fileData, dwFileSize, MPQ_COMPRESSION_ZLIB)) {
			SPDLOG_ERROR("({}) Failed to write {} bytes to {} in archive {}", GetLastError(), dwFileSize, path.c_str(), MainPath.c_str());
			if (!SFileCloseFile(hFile)) {
				SPDLOG_ERROR("({}) Failed to close file {} after write failure in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
			}
			return false;
		}

		if (!SFileFinishFile(hFile)) {
			SPDLOG_ERROR("({}) Failed to finish file {} in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
			if (!SFileCloseFile(hFile)) {
				SPDLOG_ERROR("({}) Failed to close file {} after finish failure in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
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
			SPDLOG_ERROR("({}) Failed to remove file {} in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
			return false;
		}
		
		return true;
	}

	bool OTRArchive::RenameFile(std::string oldPath, std::string newPath) {
		if (!SFileRenameFile(mainMPQ, oldPath.c_str(), newPath.c_str())) {
			SPDLOG_ERROR("({}) Failed to rename file {} to {} in archive {}", GetLastError(), oldPath.c_str(), newPath.c_str(), MainPath.c_str());
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
				else if (!fileFound && GetLastError() != ERROR_NO_MORE_FILES)
				//else if (!fileFound)
				{
					SPDLOG_ERROR("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
					if (!SListFileFindClose(hFind)) {
						SPDLOG_ERROR("({}) Failed to close file search {} after failure in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
					}
					return fileList;
				}
			} while (fileFound);
		}
		else if (GetLastError() != ERROR_NO_MORE_FILES) {
			SPDLOG_ERROR("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
			return fileList;
		}

		if (hFind != nullptr)
		{
			if (!SFileFindClose(hFind)) {
				SPDLOG_ERROR("({}) Failed to close file search {} in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
			}
		}

		return fileList;
	}

	bool OTRArchive::HasFile(std::string filename)
	{
		bool result = false;
		auto start = std::chrono::steady_clock::now();

		auto lst = ListFiles(filename);
		
		for (auto item : lst)
		{
			if (item.cFileName == filename)
			{
				result = true;
				break;
			}
		}

		auto end = std::chrono::steady_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		if (diff > 2)
			printf("HasFile call took %lims with a list of size %i\n", diff, lst.size());

		return result;
	}

	std::string OTRArchive::HashToString(uint64_t hash)
	{
		return hashes[hash];
	}

	bool OTRArchive::Load(bool genCRCMap) {
		return LoadMainMPQ(genCRCMap) && LoadPatchMPQs();
	}

	bool OTRArchive::Unload() 
	{
		bool success = true;
		for (auto mpqHandle : mpqHandles) {
			if (!SFileCloseArchive(mpqHandle.second)) {
				SPDLOG_ERROR("({}) Failed to close mpq {}", GetLastError(), mpqHandle.first.c_str());
				success = false;
			}
		}

		mainMPQ = nullptr;

		return success;
	}

	bool OTRArchive::LoadPatchMPQs() {
		// TODO: We also want to periodically scan the patch directories for new MPQs. When new MPQs are found we will load the contents to fileCache and then copy over to gameResourceAddresses
		if (PatchesPath.length() > 0) {
			for (auto& p : std::filesystem::recursive_directory_iterator(PatchesPath)) {
				if (StringHelper::IEquals(p.path().extension().string(), ".otr") || StringHelper::IEquals(p.path().extension().string(), ".mpq")) {
					if (!LoadPatchMPQ(p.path().string())) {
						return false;
					}
				}
			}
		}

		return true;
	}

	bool OTRArchive::LoadMainMPQ(bool genCRCMap) {
		HANDLE mpqHandle = NULL;

		TCHAR* t_filename = new TCHAR[MainPath.size() + 1];
		t_filename[MainPath.size()] = 0;
		std::copy(MainPath.begin(), MainPath.end(), t_filename);

		if (!SFileOpenArchive(t_filename, 0, 0 /*MPQ_OPEN_READ_ONLY*/, &mpqHandle)) {
			SPDLOG_ERROR("({}) Failed to open main mpq file {}.", GetLastError(), MainPath.c_str());
			return false;
		}

		mpqHandles[MainPath] = mpqHandle;
		mainMPQ = mpqHandle;

		if (genCRCMap)
		{
			auto listFile = LoadFile("(listfile)", false);

			std::vector<std::string> lines = StringHelper::Split(std::string(listFile->buffer.get(), listFile->dwBufferSize), "\n");

			for (int i = 0; i < lines.size(); i++)
			{
				std::string line = StringHelper::Strip(lines[i], "\r");
				//uint64_t hash = StringHelper::StrToL(lines[i], 16);

				uint64_t hash = CRC64(line.c_str());
				hashes[hash] = line;
			}
		}

		return true;
	}

	bool OTRArchive::LoadPatchMPQ(std::string path) {
		HANDLE patchHandle = NULL;

		if (!SFileOpenArchive((TCHAR*)path.c_str(), 0, MPQ_OPEN_READ_ONLY, &patchHandle)) {
			SPDLOG_ERROR("({}) Failed to open patch mpq file {} while applying to {}.", GetLastError(), path.c_str(), MainPath.c_str());
			return false;
		}

		if (!SFileOpenPatchArchive(mainMPQ, (TCHAR*)path.c_str(), "", 0)) {
			SPDLOG_ERROR("({}) Failed to apply patch mpq file {} to main mpq {}.", GetLastError(), path.c_str(), MainPath.c_str());
			return false;
		}

		mpqHandles[path] = patchHandle;

		return true;
	}
}