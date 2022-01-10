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

	void KeyboardController::Read(OSContPad* pad) {
		wStickX = 0;
		wStickY = 0;

		pad->button = dwPressedButtons & 0xFFFF;

		if (dwPressedButtons & BTN_STICKLEFT) {
			pad->stick_x = -128;
		}
		else if (dwPressedButtons & BTN_STICKRIGHT) {
			pad->stick_x = 127;
		}
		else {
			pad->stick_x = wStickX;
		}

		if (dwPressedButtons & BTN_STICKDOWN) {
			pad->stick_y = -128;
		}
		else if (dwPressedButtons & BTN_STICKUP) {
			pad->stick_y = 127;
		}
		else {
			pad->stick_y = wStickY;
		}
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