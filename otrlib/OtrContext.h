#pragma once

#include <memory>
#include "stdint.h"
#include "spdlog/spdlog.h"
#include "OTRConfigFile.h"

namespace OtrLib {
	class OTRResourceMgr;
	class OTRWindow;

	class OTRContext {
		public:
			static std::shared_ptr<OTRContext> GetInstance();
			static std::shared_ptr<OTRContext> CreateInstance(std::string Name, std::string MainPath, std::string PatchesPath);

			std::string GetName() { return Name; }
			std::shared_ptr<OTRWindow> GetWindow() { return Window; }
			std::shared_ptr<OTRResourceMgr> GetResourceManager() { return ResourceMgr; }
			std::shared_ptr<spdlog::logger> GetLogger() { return Logger; }
			std::shared_ptr<OTRConfigFile> GetConfig() { return Config; }

			OTRContext(std::string Name, std::string MainPath, std::string PatchesDirectory);
			~OTRContext();

		protected:
			void InitWindow();
			void InitLogging();

		private:
			static std::weak_ptr <OTRContext> Context;
			std::shared_ptr<spdlog::logger> Logger;
			std::shared_ptr<OTRWindow> Window;
			std::shared_ptr<OTRConfigFile> Config; // Config needs to be after the Window because we call the Window during it's destructor.
			std::shared_ptr<OTRResourceMgr> ResourceMgr;
			std::string Name;
			std::string MainPath;
			std::string PatchesPath;
	};
}
