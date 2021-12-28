#pragma once

#undef _DLL

#include <string>

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "OTRResource.h"
//#include "Lib/StrHash64.h"
#include "Lib/StormLib/StormLib.h"


namespace OtrLib
{
	class OTRFile;

	class OTRArchive : public std::enable_shared_from_this<OTRArchive>
	{
	public:
		OTRArchive();
		OTRArchive(std::string MainPath);
		OTRArchive(std::string MainPath, std::string PatchesPath);
		~OTRArchive();

		static std::shared_ptr<OTRArchive> CreateArchive(std::string archivePath);

		std::shared_ptr<OTRFile> LoadFile(std::string filePath, bool includeParent = true);

		bool AddFile(std::string path, uintptr_t fileData, DWORD dwFileSize);
		bool RemoveFile(std::string path);
		bool RenameFile(std::string oldPath, std::string newPath);
		std::vector<SFILE_FIND_DATA> ListFiles(std::string searchMask);
		bool HasFile(std::string searchMask);
		std::string HashToString(uint64_t hash);
	protected:
		bool Load();
		bool Unload();
	private:
		std::string MainPath;
		std::string PatchesPath;
		std::map<std::string, HANDLE> mpqHandles;
		std::vector<std::string> addedFiles;
		std::map<uint64_t, std::string> hashes;
		HANDLE mainMPQ;

		bool LoadMainMPQ();
		bool LoadPatchMPQs();
		bool LoadPatchMPQ(std::string path);
	};
}