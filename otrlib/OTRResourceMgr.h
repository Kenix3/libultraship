#include "Lib/StormLib/StormLib.h"
#include <map>
#include <unordered_set>
#include <string>
#include "OTRResource.h"

namespace OtrLib
{
	class OTRFile;

	class OTRResourceMgr
	{
	public:
		OTRResourceMgr();
		~OTRResourceMgr();

		void LoadArchiveAndPatches(std::string mainArchivePath, std::string patchesPath);
		void LoadPatchArchives(std::string patchesPath);
		std::shared_ptr<OTRFile> LoadFileFromCache(std::string filePath);

		DWORD LoadFile(uintptr_t destination, DWORD destinationSize, std::string filePath);
		void FreeFile(uintptr_t destination, DWORD destinationSize, std::string filePath);
		std::shared_ptr<OTRResource> LoadOTRFile(std::string filePath);
		void CacheDirectory();

	protected:
		std::map<std::string, std::shared_ptr<OTRFile>> fileCache;
		std::map<std::string, std::shared_ptr<OTRResource>> otrCache;
		std::map<std::string, std::shared_ptr<std::unordered_set<uintptr_t>>> gameResourceAddresses;
		std::map<std::string, HANDLE> mpqHandles;
		HANDLE mainMPQ;


		void LoadMPQMain(std::string filePath);
		void LoadMPQPatch(std::string patchFilePath);
	};
}