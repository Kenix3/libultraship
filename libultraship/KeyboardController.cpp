#include "KeyboardController.h"
#include "GlobalCtx2.h"
#include <SDL2/SDL_mouse.h>

float previousGyroX = 0.0f;
float previousGyroY = 0.0f;

namespace Ship {
	KeyboardController::KeyboardController(int32_t dwControllerNumber) : Controller(dwControllerNumber) {
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

		int x, y;
		SDL_GetMouseState(&x, &y);

		wGyroY = -float(x - previousGyroX) / 50.0f;
		wGyroX = -float(y - previousGyroY) / 50.0f;

		previousGyroX = x;
		previousGyroY = y;
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