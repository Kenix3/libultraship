#include "Lib/StormLib/StormLib.h"
#include <map>
#include <unordered_set>
#include <string>
#include "OTRResource.h"


namespace OtrLib
{
	class OTRArchive;
	class OTRFile;

	// Resource manager caches any and all files it comes across into memory. This will be unoptimal in the future when modifications have gigabytes of assets.
	// It works with the original game's assets because the entire ROM is 64MB and fits into RAM of any semi-modern PC.
	class OTRResourceMgr
	{
	public:
		OTRResourceMgr(std::string mainPath, std::string patchesDirectory);
		~OTRResourceMgr();

		
		DWORD LoadFile(uintptr_t destination, DWORD destinationSize, std::string filePath);
		void MarkFileAsFree(uintptr_t destination, DWORD destinationSize, std::string filePath);
		std::shared_ptr<OTRResource> LoadOTRFile(std::string filePath);
		std::shared_ptr<std::vector<std::shared_ptr<OTRFile>>> CacheDirectory(std::string searchMask);

	protected:
		std::shared_ptr<OTRFile> LoadFileFromCache(std::string filePath);

	private:
		std::map<std::string, std::shared_ptr<OTRFile>> fileCache;
		std::map<std::string, std::shared_ptr<OTRResource>> otrCache;
		std::map<std::string, std::shared_ptr<std::unordered_set<uintptr_t>>> gameResourceAddresses;
		std::shared_ptr<OTRArchive> archive;
	};
}