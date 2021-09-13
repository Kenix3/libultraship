#include "OTRResourceMgr.h"
#include "Factories/OTRResourceLoader.h"
#include <Utils/MemoryStream.h>

using namespace OtrLib;

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

// TODO: Figure out if we want to return a copy of the data or reference.
// We need to determine if teh game will ever modify the data. If this happens we need to serve a copy.
// If it doesn't, serving a reference will make for a decent optimization...
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

OTRResource* OtrLib::OTRResourceMgr::LoadOTRFile(std::string filePath)
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
			printf("ERROR CODE: %i\n", GetLastError());
			return nullptr;
		}

		int fileSize = SFileGetFileSize(fileHandle, 0);
		char* fileData = new char[fileSize];
		DWORD dwBytes = 1;

		if (!SFileReadFile(fileHandle, fileData, fileSize, &dwBytes, NULL))
			return nullptr;

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

void OTRResourceMgr::LoadMPQPatch(std::string patchFilePath)
{
	HANDLE mpqHandle2 = NULL;

	if (!SFileOpenArchive((TCHAR*)patchFilePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &mpqHandle2))
	{
		printf("ERROR CODE: %i\n", GetLastError());
		return;
	}

	if (!SFileOpenPatchArchive(mainMPQ, (TCHAR*)patchFilePath.c_str(), "", 0))
	{
		printf("ERROR CODE: %i\n", GetLastError());
		return;
	}

	mpqHandles[patchFilePath] = mpqHandle2;
}
