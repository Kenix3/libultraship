#pragma once

#include <memory>
#include <map>
#include <string>
#include "stdint.h"
#include "UltraController.h"
#include "OTRControllerAttachment.h"

#define EXTENDED_SCANCODE_BIT (1 << 8)
#define AXIS_SCANCODE_BIT (1 << 9)

namespace OtrLib {
	class OTRController {
		friend class OTRWindow;

		public:
			OTRController(int32_t dwControllerNumber);

			virtual void Read(OSContPad* pad) = 0;

			void SetButtonMapping(std::string szButtonName, int32_t dwScancode);
			std::shared_ptr<OTRControllerAttachment> GetAttachment() { return Attachment; }
			int32_t GetControllerNumber() { return dwControllerNumber; }

		protected:
			int32_t dwPressedButtons;
			std::map<int32_t, int32_t> ButtonMapping;
			int8_t wStickX;
			int8_t wStickY;
			
			virtual std::string GetControllerType() = 0;
			virtual std::string GetConfSection() = 0;
			virtual std::string GetBindingConfSection() = 0;
			void LoadBinding();

		private:
			std::shared_ptr<OTRControllerAttachment> Attachment;
			int32_t dwControllerNumber;
	};
}
