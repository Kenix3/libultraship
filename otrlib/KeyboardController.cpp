#include "KeyboardController.h"
#include <SDL2/SDL.h>
#include "spdlog/spdlog.h"
#include "OTRContext.h"

namespace OtrLib {
	KeyboardController::KeyboardController(int32_t dwControllerNumber) : OTRController(dwControllerNumber) {
		LoadConfig();
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

	void KeyboardController::LoadConfig() {
		std::string ConfSection = GetConfSection();
		std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
		OTRConfigFile& Conf = *pConf.get();

		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_CRIGHT)])] = BTN_CRIGHT;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_CLEFT)])] = BTN_CLEFT;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_CDOWN)])] = BTN_CDOWN;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_CUP)])] = BTN_CUP;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_R)])] = BTN_R;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_L)])] = BTN_L;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_DRIGHT)])] = BTN_DRIGHT;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_DLEFT)])] = BTN_DLEFT;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_DDOWN)])] = BTN_DDOWN;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_DUP)])] = BTN_DUP;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_START)])] = BTN_START;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_Z)])] = BTN_Z;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_B)])] = BTN_B;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_A)])] = BTN_A;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_STICKRIGHT)])] = BTN_STICKRIGHT;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_STICKLEFT)])] = BTN_STICKLEFT;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_STICKDOWN)])] = BTN_STICKDOWN;
		ButtonMapping[std::stoi(Conf[ConfSection][STR(BTN_STICKUP)])] = BTN_STICKUP;
	}

	void KeyboardController::SetButtonMapping(std::string szButtonName, int32_t dwScancode) {
		// Update the config value.
		std::string ConfSection = GetConfSection();
		std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
		OTRConfigFile& Conf = *pConf.get();
		Conf[ConfSection][STR(BTN_CRIGHT)] = dwScancode;
		
		// Reload the button mapping from Config
		LoadConfig();
	}

	std::string KeyboardController::GetConfSection() {
		return "KEYBOARD CONTROLLER " + std::to_string(GetControllerNumber());
	}
}