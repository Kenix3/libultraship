#pragma once

#include <string>
#include <memory>
#include "GlobalCtx2.h"

namespace Ship {
	class Archive;

	class File
	{
	public:
		std::shared_ptr<Archive> parent;
		std::string path;
		std::shared_ptr<char[]> buffer;
		uint32_t dwBufferSize;
		volatile bool bIsLoaded = false;
		volatile bool bHasLoadError = false;
		volatile bool bHasResourceLoaded = false;
		std::condition_variable FileLoadNotifier;
		std::mutex FileLoadMutex;
		std::condition_variable ResourceLoadNotifier;
		std::mutex ResourceLoadMutex;
	};
}
