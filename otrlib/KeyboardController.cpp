#include "KeyboardController.h"
#include "OTRContext.h"

namespace OtrLib {
	KeyboardController::KeyboardController(int32_t dwControllerNumber) : OTRController(dwControllerNumber) {
		LoadBinding();
	}

	KeyboardController::~KeyboardController() {
		
	}

	bool KeyboardController::PressButton(int32_t dwScancode) {
		if (ButtonMapping.contains(dwScancode)) {
			dwPressedButtons |= ButtonMapping[dwScancode];
			return true;
		}

		return false;
	}

	bool KeyboardController::ReleaseButton(int32_t dwScancode) {
		if (ButtonMapping.contains(dwScancode)) {
			dwPressedButtons &= ~ButtonMapping[dwScancode];
			return true;
		}

		return false;
	}

	void KeyboardController::ReleaseAllButtons() {
		dwPressedButtons = 0;
	}

	void KeyboardController::ReadFromSource() {
		wStickX = 0;
		wStickY = 0;
	}

	std::string KeyboardController::GetControllerType() {
		return "KEYBOARD";
	}

	std::string KeyboardController::GetConfSection() {
		return GetControllerType() + " CONTROLLER " + std::to_string(GetControllerNumber() + 1);
	}

	std::string KeyboardController::GetBindingConfSection() {
		return GetControllerType() + " CONTROLLER BINDING " + std::to_string(GetControllerNumber() + 1);
	}
}