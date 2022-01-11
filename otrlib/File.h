#pragma once
#include "Lib/StormLib/StormLib.h"
#include <string>
#include <memory>

namespace Ship {
	class Archive;

	class File
	{
	public:
		std::shared_ptr<Archive> parent;
		std::string path;
		std::shared_ptr<char[]> buffer;
		DWORD dwBufferSize;
	};
}
