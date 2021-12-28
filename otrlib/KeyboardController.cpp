#include "KeyboardController.h"
#include <SDL2/SDL.h>
#include <map>
#include "spdlog/spdlog.h"

namespace OtrLib {
	std::map<int32_t, int32_t> KeyboardController::GetDefaultMapping(int32_t dwControllerNumber) {
		std::map<int32_t, int32_t> Defaults = std::map<int32_t, int32_t>();
		switch (dwControllerNumber) {
			case 0:
				Defaults[0x14D] = BTN_CRIGHT;
				Defaults[0x14B] = BTN_CLEFT;
				Defaults[0x150] = BTN_CDOWN;
				Defaults[0x148] = BTN_CUP;
				Defaults[0x036] = BTN_R;
				Defaults[0x035] = BTN_L;
				Defaults[0x023] = BTN_DRIGHT;
				Defaults[0x021] = BTN_DLEFT;
				Defaults[0x022] = BTN_DDOWN;
				Defaults[0x014] = BTN_DUP;
				Defaults[0x039] = BTN_START;
				Defaults[0x025] = BTN_Z;
				Defaults[0x033] = BTN_B;
				Defaults[0x026] = BTN_A;
				Defaults[0x01E] = BTN_STICKLEFT;
				Defaults[0x020] = BTN_STICKRIGHT;
				Defaults[0x01F] = BTN_STICKDOWN;
				Defaults[0x011] = BTN_STICKUP;
				break;
		}
		return Defaults;
	}


	KeyboardController::KeyboardController(int32_t dwControllerNumber) : OTRController(dwControllerNumber) {
		dwPressedButtons = 0;

		// TODO: Setup mappings from data. This is just the default from the SM64 guys. Note that L and the dpad have no values set.
		if (dwControllerNumber == 0) {
			ButtonMapping = GetDefaultMapping(dwControllerNumber);
		}
	}

	KeyboardController::~KeyboardController() {
		
	}

	void KeyboardController::Read(OSContPad* pad) {
		pad->button = dwPressedButtons & 0xFFFFF;

		if (dwPressedButtons & BTN_STICKLEFT) {
			pad->stick_x = -128;
		}
		if (dwPressedButtons & BTN_STICKRIGHT) {
			pad->stick_x = 127;
		}
		if (dwPressedButtons & BTN_STICKDOWN) {
			pad->stick_y = -128;
		}
		if (dwPressedButtons & BTN_STICKUP) {
			pad->stick_y = 127;
		}
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

	void KeyboardController::SetButtonMapping(int32_t dwN64Button, int32_t dwScancode) {
		ButtonMapping[dwN64Button] = dwScancode;
	}
}