#pragma once
#include "OTRController.h"
#include <string>

namespace OtrLib {
	class KeyboardController : public OTRController {
		public:
			KeyboardController(int32_t dwControllerNumber);
			~KeyboardController();

			void Read(OSContPad* pad);
			bool PressButton(int32_t dwScancode);
			bool ReleaseButton(int32_t dwScancode);
			void ReleaseAllButtons();

		protected:
			std::string GetControllerType();
			std::string GetConfSection();
			std::string GetBindingConfSection();
	};
}
