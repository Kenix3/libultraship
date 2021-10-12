#pragma once

#include <string>

#include <stdint.h>
#include "OTRResource.h"
#include "Lib/StormLib/StormLib.h"

namespace OtrLib
{
	class OTRArchive
	{
	public:
		OTRArchive(std::string filePath);
		~OTRArchive();
	private:
		std::string filePath;
		HANDLE mpq;
	};
}