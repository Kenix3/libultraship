#pragma once

#include <string>
#include <memory>
#include "Lib/mINI/src/mini/ini.h"
#include "UltraController.h"
#include "OTRMacros.h"

namespace OtrLib {
	class OTRContext;

	class OTRConfigFile {
		public:
			OTRConfigFile(std::shared_ptr<OTRContext> Context, std::string Path);
			~OTRConfigFile();
			
			bool Save();

			// Expose the ini values.
			mINI::INIMap<std::string>& operator[](std::string Section);
			mINI::INIMap<std::string> get(std::string Section);
			bool has(std::string Section);
			bool remove(std::string Section);
			void clear();
			std::size_t size() const;

		protected:
			bool CreateDefaultConfig();

		private:
			mINI::INIFile File;
			mINI::INIStructure Val;
			std::shared_ptr<OTRContext> Context;
			std::string Path;
	};
}
