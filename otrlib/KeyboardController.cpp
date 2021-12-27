#include "KeyboardController.h"
#include <SDL2/SDL.h>
#include <map>
#include "spdlog/spdlog.h"

namespace OtrLib {
	std::map<int32_t, int32_t> KeyboardController::GetDefaultMapping(int32_t dwControllerNumber) {
		std::map<int32_t, int32_t> Defaults = std::map<int32_t, int32_t>();
		switch (dwControllerNumber) {
			case 0:
				Defaults[SDL_Scancode::SDL_SCANCODE_RIGHT] = BTN_CRIGHT;
				Defaults[SDL_Scancode::SDL_SCANCODE_LEFT] = BTN_CLEFT;
				Defaults[SDL_Scancode::SDL_SCANCODE_DOWN] = BTN_CDOWN;
				Defaults[SDL_Scancode::SDL_SCANCODE_UP] = BTN_CUP;
				Defaults[SDL_Scancode::SDL_SCANCODE_RSHIFT] = BTN_R;
				Defaults[SDL_Scancode::SDL_SCANCODE_SLASH] = BTN_L;
				Defaults[SDL_Scancode::SDL_SCANCODE_H] = BTN_DRIGHT;
				Defaults[SDL_Scancode::SDL_SCANCODE_F] = BTN_DLEFT;
				Defaults[SDL_Scancode::SDL_SCANCODE_G] = BTN_DDOWN;
				Defaults[SDL_Scancode::SDL_SCANCODE_T] = BTN_DUP;
				Defaults[SDL_Scancode::SDL_SCANCODE_T] = BTN_DUP;
				Defaults[SDL_Scancode::SDL_SCANCODE_SPACE] = BTN_START;
				Defaults[SDL_Scancode::SDL_SCANCODE_K] = BTN_Z;
				Defaults[SDL_Scancode::SDL_SCANCODE_COMMA] = BTN_B;
				Defaults[SDL_Scancode::SDL_SCANCODE_L] = BTN_A;
				Defaults[SDL_Scancode::SDL_SCANCODE_A] = BTN_STICKLEFT;
				Defaults[SDL_Scancode::SDL_SCANCODE_D] = BTN_STICKRIGHT;
				Defaults[SDL_Scancode::SDL_SCANCODE_S] = BTN_STICKDOWN;
				Defaults[SDL_Scancode::SDL_SCANCODE_W] = BTN_STICKUP;
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

	void KeyboardController::UpdateButtons() {
		int32_t dwKeyCount;
		auto keys = SDL_GetKeyboardState(&dwKeyCount);

		for (const auto& [dwScancode, dwButtonMask] : ButtonMapping) {
			if (dwScancode < dwKeyCount) {
				UpdateButton(dwScancode, keys[dwScancode]);
			}
		}
	}

	void KeyboardController::UpdateButton(int32_t dwScancode, bool bIsPressed) {
		int32_t dwButtonMask = ButtonMapping[dwScancode];
		
		if (bIsPressed) {
			dwPressedButtons |= dwButtonMask;
		} else {
			dwPressedButtons &= ~dwButtonMask;
		}
	}
}