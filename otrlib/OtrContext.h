#pragma once

#include "stdint.h"
#include "spdlog/spdlog.h"
#include <memory>

namespace OtrLib {
	class OTRResourceMgr;

	class OTRContext {
		public:
			std::shared_ptr<OTRResourceMgr> ResourceMgr;

			OTRContext(std::string mainPath, std::string patchesDirectory);
			~OTRContext();

		protected:

		private:
			std::shared_ptr<spdlog::logger> Logger;
			std::string Name;
			int32_t contextId;
	};
}
