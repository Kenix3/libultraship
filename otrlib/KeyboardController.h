#pragma once
#include "OTRController.h"
#include <map>

namespace OtrLib {
	class KeyboardController : public OTRController {
		friend class OTRWindow;

		public:
			static std::map<int32_t, int32_t> GetDefaultMapping(int32_t dwControllerNumber);

			KeyboardController(int32_t dwControllerNumber);
			~KeyboardController();
			void Read(OSContPad* pad);
			void SetButtonMapping(int32_t dwN64Button, int32_t dwScancode);

		protected:

		private:
			bool PressButton(int32_t dwScancode);
			bool ReleaseButton(int32_t dwScancode);
			void ReleaseAllButtons();

			std::map<int32_t, int32_t> ButtonMapping;
	};
}
