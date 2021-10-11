#include "Lib/StormLib/StormLib.h"
#include <map>
#include <string>
#include "OTRResource.h"

namespace OtrLib
{
	class OTRResourceMgr
	{
	public:
		OTRResourceMgr();

		void LoadArchiveAndPatches(std::string mainArchivePath, std::string patchesPath);

		char* LoadFile(std::string filePath);
		OTRResource* LoadOTRFile(std::string filePath);
		void CacheDirectory();

	protected:
		std::map<std::string, char*> fileCache; // TODO: Use smart pointers here...
		std::map<std::string, OTRResource*> otrCache; // TODO: Use smart pointers here...
		std::map<std::string, HANDLE> patchMpqHandles;
		HANDLE mainMPQ;


		void LoadMPQ(std::string filePath);
		void LoadMPQPatch(std::string patchFilePath);
	};
}