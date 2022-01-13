#pragma once

#include <string>
#include <memory>
#include "Lib/mINI/src/mini/ini.h"
#include "UltraController.h"
#include "LUSMacros.h"

namespace Ship {
	class GlobalCtx2;

	class ConfigFile {
		public:
			ConfigFile(std::shared_ptr<GlobalCtx2> Context, std::string Path);
			~ConfigFile();
			
			bool Save();

			// Expose the ini values.
			mINI::INIMap<std::string>& operator[](std::string Section);
			mINI::INIMap<std::string> get(std::string Section);
			bool has(std::string Section);
			bool remove(std::string Section);
			void clear();
			std::size_t size() const;
			std::shared_ptr<GlobalCtx2> GetContext() { return Context.lock(); }

		protected:
			bool CreateDefaultConfig();

		private:
			mINI::INIFile File;
			mINI::INIStructure Val;
			std::weak_ptr<GlobalCtx2> Context;
			std::string Path;
	};
}
