#pragma once

#include <string>

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "OTRResource.h"
#include "Lib/StormLib/StormLib.h"


namespace OtrLib
{
	class OTRFile;

	class OTRArchive
	{
	public:
		OTRArchive(std::string mainPath);
		OTRArchive(std::string mainPath, std::string patchesDirectory);
		~OTRArchive();

		std::shared_ptr<OTRFile> LoadFile(std::string filePath);

		bool AddFile(std::string path, uintptr_t fileData, DWORD dwFileSize);
		bool RemoveFile(std::string path);
		bool RenameFile(std::string oldPath, std::string newPath);
		std::shared_ptr<std::vector<SFILE_FIND_DATA>> ListFiles(std::string searchMask);
	protected:
		bool Load();
		bool Unload();
	private:
		std::string mainPath;
		std::string patchesDirectory;
		std::map<std::string, HANDLE> mpqHandles;
		HANDLE mainMPQ;

		bool LoadMainMPQ();
		bool LoadPatchMPQs();
		bool LoadPatchMPQ(std::string path);
	};
}