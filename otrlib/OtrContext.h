#pragma once

#include "spdlog/spdlog.h"
#include "stdint.h"

class OTRResourceMgr;


namespace OtrLib {
	class OtrContext
	{
	public:
		OtrContext();
		~OtrContext();
	private:
		std::shared_ptr<OTRResourceMgr> ResourceMgr;
		std::shared_ptr<spdlog::logger> Logger;
		std::string Name;
		int32_t contextId;
	};
}
