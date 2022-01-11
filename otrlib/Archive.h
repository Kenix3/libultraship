#pragma once

#undef _DLL

#include <string>

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "Resource.h"
//#include "Lib/StrHash64.h"
#include "Lib/StormLib/StormLib.h"


namespace Ship
{
	class File;

	class Archive : public std::enable_shared_from_this<Archive>
	{
	public:
		Archive();
		Archive(std::string MainPath);
		Archive(std::string MainPath, std::string PatchesPath);
		~Archive();

		static std::shared_ptr<Archive> CreateArchive(std::string archivePath);

		std::shared_ptr<File> LoadFile(std::string filePath, bool includeParent = true);

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