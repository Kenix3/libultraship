#pragma once
#include "OTRController.h"
#include <map>

namespace OtrLib {
	class KeyboardController : public OTRController {
		friend class OTRInput;

		public:
			static std::map<int32_t, int32_t> GetDefaultMapping(int32_t dwControllerNumber);

			KeyboardController(int32_t dwControllerNumber);
			~KeyboardController();
			void Read(OSContPad* pad);

		protected:

		private:
			void UpdateButtons();
			void UpdateButton(int32_t dwScancode, bool bIsPressed);

			int32_t dwPressedButtons;
			std::map<int32_t, int32_t> ButtonMapping;
	};
}
