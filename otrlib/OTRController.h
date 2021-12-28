#pragma once

#include "stdint.h"
#include "UltraController.h"

namespace OtrLib {
	class OTRController {
		public:
			OTRController(int32_t dwControllerNumber);
			virtual void Read(OSContPad* pad);

			int32_t GetContollerNumber() { return dwControllerNumber; }

		protected:
			int32_t dwPressedButtons;
			int32_t dwControllerNumber;
	};
}
