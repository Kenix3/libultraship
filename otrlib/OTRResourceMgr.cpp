#include "OTRResourceMgr.h"
#include "Factories/OTRResourceLoader.h"
#include <Utils/MemoryStream.h>
#include "spdlog/spdlog.h"

namespace OtrLib {

	OTRResourceMgr::OTRResourceMgr()
	{
		mainMPQ = NULL;
	}

	void OTRResourceMgr::LoadArchiveAndPatches(std::string mainArchiveFilePath, std::string patchesPath)
	{
		LoadMPQMain(mainArchiveFilePath.c_str());

		if (patchesPath.length() > 0) {
			// TODO: Does directory exist?
			// TODO: This needs to be something like a directory search for all .otr files in the patches folder.
			/*
			// Make this do a directory search...
			LoadMPQPatch("Patch1.MPQ");
			LoadMPQPatch("Patch2.MPQ");
			LoadMPQPatch("Patch3.MPQ");
			*/
		}
	}

	// TODO: Accept in a buffer and a buffer size. Then copy over the data. Save that buffer in a list for the resoruce name.
	// TODO: We want an internal load file that will just load the file to resource manager cache, and a separate API load file to copy to the game's buffer. A third API function will make an OTRResource from that.
	char* OTRResourceMgr::LoadFile(std::string filePath)
	{
		// File already loaded...?
		if (fileCache.find(filePath) != fileCache.end())
		{
			return fileCache[filePath];
		}
		else
		{
			HANDLE fileHandle = NULL;

			if (!SFileOpenFileEx(mainMPQ, filePath.c_str(), 0, &fileHandle))
			{
				spdlog::error("Failed to load file from mpq, ERROR CODE: {}", GetLastError());
				return nullptr;
			}

			int fileSize = SFileGetFileSize(fileHandle, 0);
			char* fileData = new char[fileSize];
			DWORD dwBytes = 1;

			if (!SFileReadFile(fileHandle, fileData, fileSize, &dwBytes, NULL))
			{
				// TODO: Close file handle
				spdlog::error("Failed to read file from mpq, ERROR CODE: {}", GetLastError());
				return nullptr;
			}

			// TODO: Close file handle

			fileCache[filePath] = fileData;
			return fileData;
		}
	}

	OTRResource* OTRResourceMgr::LoadOTRFile(std::string filePath)
	{
		if (otrCache.find(filePath) != otrCache.end())
		{
			return otrCache[filePath];
		}
		else
		{
			HANDLE fileHandle = NULL;

			if (!SFileOpenFileEx(mainMPQ, filePath.c_str(), 0, &fileHandle))
			{
				spdlog::error("Failed to load file from mpq, ERROR CODE : {}", GetLastError());
				return nullptr;
			}

			int fileSize = SFileGetFileSize(fileHandle, 0);
			char* fileData = new char[fileSize];
			DWORD dwBytes = 1;

			if (!SFileReadFile(fileHandle, fileData, fileSize, &dwBytes, NULL)) {
				// TODO: Close file handle
				spdlog::error("Failed to read file from mpq, ERROR CODE: {}", GetLastError());
				return nullptr;
			}

			// TODO: Close the file handle

			MemoryStream memStream = MemoryStream(fileData, fileSize);
			BinaryReader reader = BinaryReader(&memStream);
			OTRResource* res = OTRResourceLoader::LoadResource(&reader);

			if (res != nullptr)
				otrCache[filePath] = res;

			return res;
		}

		return nullptr;
	}

	void OTRResourceMgr::CacheDirectory()
	{
		// TODO: Figure out how "searching" works with StormLib...
	}

	void OTRResourceMgr::LoadMPQMain(std::string filePath)
	{
		HANDLE mpqHandle = NULL;

		if (!SFileOpenArchive((TCHAR*)filePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle))
		{
			spdlog::error("Failed to open main mpq file {}. ERROR CODE: {}", filePath.c_str(), GetLastError());
			return;
		}

		mpqHandles[filePath] = mpqHandle;
		mainMPQ = mpqHandle;
	}

	void OTRResourceMgr::LoadMPQPatch(std::string patchFilePath)
	{
		HANDLE mpqHandle2 = NULL;

		if (!SFileOpenArchive((TCHAR*)patchFilePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle2))
		{
			spdlog::error("Failed to open patch mpq file {}. ERROR CODE: {}", patchFilePath.c_str(), GetLastError());
			return;
		}

		if (!SFileOpenPatchArchive(mainMPQ, (TCHAR*)patchFilePath.c_str(), "", 0))
		{
			spdlog::error("Failed to apply patch mpq file {}. ERROR CODE: {}", patchFilePath.c_str(), GetLastError());
			return;
		}

		mpqHandles[patchFilePath] = mpqHandle2;
	}
}