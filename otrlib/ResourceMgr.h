#include <map>
#include <unordered_set>
#include <string>
#include "Resource.h"
#include "GlobalCtx2.h"

namespace Ship
{
	class Archive;
	class File;

	// Resource manager caches any and all files it comes across into memory. This will be unoptimal in the future when modifications have gigabytes of assets.
	// It works with the original game's assets because the entire ROM is 64MB and fits into RAM of any semi-modern PC.
	class ResourceMgr
	{
	public:
		ResourceMgr(std::shared_ptr<GlobalCtx2> Context, std::string MainPath, std::string PatchesPath);
		~ResourceMgr();

		std::shared_ptr<GlobalCtx2> GetContext() { return Context.lock(); }
		char* LoadFileOriginal(std::string filePath);
		DWORD LoadFile(uintptr_t destination, DWORD destinationSize, std::string filePath);
		void MarkFileAsFree(uintptr_t destination, DWORD destinationSize, std::string filePath);
		std::string HashToString(uint64_t hash);
		std::shared_ptr<Resource> LoadOTRFile(std::string filePath);
		std::shared_ptr<std::vector<std::shared_ptr<File>>> CacheDirectory(std::string searchMask);

	protected:
		std::shared_ptr<File> LoadFileFromCache(std::string filePath);

	private:
		std::weak_ptr<GlobalCtx2> Context;
		std::map<std::string, std::shared_ptr<File>> FileCache;
		std::map<std::string, std::shared_ptr<Resource>> Cache;
		std::map<std::string, std::shared_ptr<std::unordered_set<uintptr_t>>> gameResourceAddresses;
		std::shared_ptr<Archive> archive;
	};
}