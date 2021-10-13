#pragma once
#include "Lib/StormLib/StormLib.h"
#include <string>
#include <memory>

namespace OtrLib {
	class OTRArchive;

	class OTRFile
	{
	public:
		std::shared_ptr<OTRArchive> parent;
		std::string path;
		std::shared_ptr<char[]> buffer;
		DWORD dwBufferSize;
	};
}
