#pragma once
#include "OTRController.h"
#include <map>
#include <string>

namespace OtrLib {
	class KeyboardController : public OTRController {
		friend class OTRWindow;

		public:
			KeyboardController(int32_t dwControllerNumber);
			~KeyboardController();
			void Read(OSContPad* pad);
			void SetButtonMapping(std::string szButtonName, int32_t dwScancode);

		protected:

		private:
			bool PressButton(int32_t dwScancode);
			bool ReleaseButton(int32_t dwScancode);
			void ReleaseAllButtons();
			std::string GetConfSection();
			void LoadConfig();

			std::map<int32_t, int32_t> ButtonMapping;
	};
}
