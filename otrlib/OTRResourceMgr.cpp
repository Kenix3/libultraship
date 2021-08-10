#include "OTRResourceMgr.h"

OTRResourceMgr::OTRResourceMgr()
{
	mainMPQ = NULL;
}

void OTRResourceMgr::LoadArchiveAndPatches()
{
	LoadMPQ("Main.MPQ");

	// Make this do a directory search...
	LoadMPQPatch("Patch1.MPQ");
	LoadMPQPatch("Patch2.MPQ");
	LoadMPQPatch("Patch3.MPQ");
}

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
			printf("ERROR CODE: %i\n", GetLastError());
			return nullptr;
		}

		int fileSize = SFileGetFileSize(fileHandle, 0);
		char* fileData = new char[fileSize];
		DWORD dwBytes = 1;

		if (!SFileReadFile(fileHandle, fileData, fileSize, &dwBytes, NULL))
		{
			return nullptr;
		}

		fileCache[filePath] = fileData;
		return fileData;
	}
}

void OTRResourceMgr::CacheDirectory()
{

}

void OTRResourceMgr::LoadMPQ(std::string filePath)
{
	HANDLE mpqHandle = NULL;

	if (!SFileOpenArchive((TCHAR*)filePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle))
	{
		printf("ERROR CODE: %i\n", GetLastError());
		return;
	}

	mpqHandles[filePath] = mpqHandle;
	mainMPQ = mpqHandle;
}

void OTRResourceMgr::LoadMPQPatch(std::string filePath)
{
	HANDLE mpqHandle2 = NULL;

	if (!SFileOpenArchive((TCHAR*)filePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle2))
	{
		printf("ERROR CODE: %i\n", GetLastError());
		return;
	}

	if (!SFileOpenPatchArchive(mainMPQ, L"new_arch2.mpq", "", 0))
	{
		printf("ERROR CODE: %i\n", GetLastError());
		return;
	}

	mpqHandles[filePath] = mpqHandle2;
}
