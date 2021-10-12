#pragma once

#include "spdlog/spdlog.h"
#include "stdint.h"


namespace OtrLib {
	class OTRResourceMgr;

	class OTRContext
	{
	public:
		OTRContext();
		~OTRContext();
	private:
		std::shared_ptr<OTRResourceMgr> ResourceMgr;
		std::shared_ptr<spdlog::logger> Logger;
		std::string Name;
		int32_t contextId;
	};
}
