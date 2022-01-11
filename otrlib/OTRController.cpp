#include "OtrController.h"
#include "OTRContext.h"
#include "stox.h"
#include <memory>

namespace OtrLib {
	OTRController::OTRController(int32_t dwControllerNumber) : dwControllerNumber(dwControllerNumber) {
		dwPressedButtons = 0;
		Attachment = nullptr;
	}

	void OTRController::Read(OSContPad* pad) {
		ReadFromSource();

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

	void OTRController::SetButtonMapping(std::string szButtonName, int32_t dwScancode) {
		// Update the config value.
		std::string ConfSection = GetBindingConfSection();
		std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
		OTRConfigFile& Conf = *pConf.get();
		Conf[ConfSection][szButtonName] = dwScancode;

		// Reload the button mapping from Config
		LoadBinding();
	}

	void OTRController::LoadBinding() {
		std::string ConfSection = GetBindingConfSection();
		std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
		OTRConfigFile& Conf = *pConf.get();

		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_CRIGHT)])] = BTN_CRIGHT;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_CLEFT)])] = BTN_CLEFT;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_CDOWN)])] = BTN_CDOWN;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_CUP)])] = BTN_CUP;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_R)])] = BTN_R;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_L)])] = BTN_L;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_DRIGHT)])] = BTN_DRIGHT;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_DLEFT)])] = BTN_DLEFT;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_DDOWN)])] = BTN_DDOWN;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_DUP)])] = BTN_DUP;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_START)])] = BTN_START;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_Z)])] = BTN_Z;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_B)])] = BTN_B;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_A)])] = BTN_A;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_STICKRIGHT)])] = BTN_STICKRIGHT;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_STICKLEFT)])] = BTN_STICKLEFT;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_STICKDOWN)])] = BTN_STICKDOWN;
		ButtonMapping[OtrLib::stoi(Conf[ConfSection][STR(BTN_STICKUP)])] = BTN_STICKUP;
	}

	std::string OTRController::GetConfSection() {
		return GetControllerType() + " CONTROLLER " + std::to_string(GetControllerNumber() + 1);
	}

	std::string OTRController::GetBindingConfSection() {
		return GetControllerType() + " CONTROLLER BINDING " + std::to_string(GetControllerNumber() + 1);
	}
}