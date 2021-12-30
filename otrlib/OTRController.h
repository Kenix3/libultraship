#pragma once

#include <memory>
#include "stdint.h"
#include "UltraController.h"
#include "OTRControllerAttachment.h"
#include "OTRRumblePack.h"
#include "OTRMemoryPack.h"

namespace OtrLib {
	class OTRController {
		public:
			OTRController(int32_t dwControllerNumber);

			virtual void Read(OSContPad* pad) = 0;

			std::shared_ptr<OTRControllerAttachment> GetAttachment() { return Attachment; }
			int32_t GetControllerNumber() { return dwControllerNumber; }

		protected:
			int32_t dwPressedButtons;

		private:
			std::shared_ptr<OTRControllerAttachment> Attachment;
			int32_t dwControllerNumber;
	};
}
