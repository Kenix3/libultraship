#include "Lib/StormLib/StormLib.h"
#include <map>
#include <string>

class OTRResourceMgr
{
public:
	OTRResourceMgr();

	void LoadArchiveAndPatches();

	char* LoadFile(std::string filePath);
	void CacheDirectory();

protected:
	std::map<std::string, char*> fileCache; // TODO: Use smart pointers here...
	std::map<std::string, HANDLE> mpqHandles;
	HANDLE mainMPQ;


	void LoadMPQ(std::string filePath);
	void LoadMPQPatch(std::string filePath);
};