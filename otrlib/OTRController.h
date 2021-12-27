#pragma once

#include "stdint.h"
#include "UltraController.h"

namespace OtrLib {
	class OTRController {
		public:
			OTRController(int32_t dwControllerNumber);
			virtual void Read(OSContPad* pad);

			int32_t dwControllerNumber;
	};
}
