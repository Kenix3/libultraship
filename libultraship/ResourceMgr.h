#pragma once

#include <map>
#include <string>
#include <thread>
#include <queue>
#include "Resource.h"
#include "GlobalCtx2.h"

namespace Ship
{
	class Archive;
	class File;

	// Resource manager caches any and all files it comes across into memory. This will be unoptimal in the future when modifications have gigabytes of assets.
	// It works with the original game's assets because the entire ROM is 64MB and fits into RAM of any semi-modern PC.
	class ResourceMgr {
	public:
		ResourceMgr(std::shared_ptr<GlobalCtx2> Context, std::string MainPath, std::string PatchesPath);
		~ResourceMgr();

		bool IsRunning();

		std::shared_ptr<GlobalCtx2> GetContext() { return Context.lock(); }
		std::string HashToString(uint64_t Hash);
		std::shared_ptr<Resource> LoadResource(std::string FilePath);
		std::shared_ptr<std::vector<std::shared_ptr<File>>> CacheDirectory(std::string SearchMask);

	protected:
		void Start();
		void Stop();
		void Run();
		void CacheFileFromArchive(std::shared_ptr<File> ToLoad);
		std::shared_ptr<File> LoadFile(std::string FilePath, bool Blocks = true);

	private:
		std::weak_ptr<GlobalCtx2> Context;
		std::map<std::string, std::shared_ptr<File>> FileCache;
		std::map<std::string, std::shared_ptr<Resource>> ResourceCache;
		std::queue<std::shared_ptr<File>> FileLoadQueue;
		std::shared_ptr<Archive> OTR;
		std::shared_ptr<std::thread> Thread;
		std::mutex Mutex;
		std::condition_variable Notifier;
		volatile bool bIsRunning;
	};
}