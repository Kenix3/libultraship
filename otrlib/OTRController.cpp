#include "OtrController.h"
#include <memory>

namespace OtrLib {
	OTRController::OTRController(int32_t dwControllerNumber) : dwControllerNumber(dwControllerNumber) {
		dwPressedButtons = 0;
		Attachment = nullptr;
	}
}