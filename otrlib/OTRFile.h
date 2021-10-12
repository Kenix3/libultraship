#pragma once
#include "Lib/StormLib/StormLib.h"
#include <memory>

namespace OtrLib {
	class OTRFile
	{
	public:
		std::shared_ptr<char[]> buffer;
		DWORD dwBufferSize;
	};
}
