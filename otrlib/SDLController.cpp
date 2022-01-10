#include "SDLController.h"
#include "OtrContext.h"
#include "spdlog/spdlog.h"
#include "stox.h"

namespace OtrLib {
	SDLController::SDLController(int32_t dwControllerNumber) : OTRController(dwControllerNumber), Cont(nullptr), guid(INVALID_SDL_CONTROLLER_GUID) {

	}

	SDLController::~SDLController() {
        Close();
	}

    bool SDLController::Open() {
        std::string ConfSection = GetConfSection();
        std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
        OTRConfigFile& Conf = *pConf.get();

        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                // Get the GUID from SDL
                char buf[33];
                SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), buf, sizeof(buf));
                auto guid = std::string(buf);

                // If the GUID read from SDL is blank, then we abort.
                if (guid.compare(INVALID_SDL_CONTROLLER_GUID) == 0) {
                    SPDLOG_ERROR("SDL Controller returned invalid guid");
                    continue;
                }

                // If the GUID is blank from the config, OR if the config GUID matches, load the controller.
                if (Conf[ConfSection]["GUID"].compare("") == 0 || Conf[ConfSection]["GUID"].compare(INVALID_SDL_CONTROLLER_GUID) == 0 || Conf[ConfSection]["GUID"].compare(guid) == 0) {
                    auto Cont = SDL_GameControllerOpen(i);

                    // We failed to load the controller. Abort.
                    if (Cont == nullptr) {
                        SPDLOG_ERROR("SDL Controller failed to open: ({})", SDL_GetError());
                        continue;
                    }

                    std::string BindingConfSection = GetBindingConfSection();
                    std::shared_ptr<OTRConfigFile> pBindingConf = OTRContext::GetInstance()->GetConfig();
                    OTRConfigFile& BindingConf = *pBindingConf.get();

                    if (!BindingConf.has(BindingConfSection)) {
                        CreateDefaultBinding(guid);
                    }

                    LoadBinding();
                    LoadAxisThresholds();

                    this->guid = guid;
                    this->Cont = Cont;
                    break;
                }
            }
        }

        return Cont != nullptr;
    }

    bool SDLController::Close() {
        if (Cont != nullptr) {
            SDL_GameControllerClose(Cont);
        }
        Cont = nullptr;
        guid = "";
        ButtonMapping.clear();
        ThresholdMapping.clear();
        dwPressedButtons = 0;
        wStickX = 0;
        wStickY = 0;

        return true;
    }

    void SDLController::LoadAxisThresholds() {
        std::string ConfSection = GetBindingConfSection();
        std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
        OTRConfigFile& Conf = *pConf.get();

        ThresholdMapping[SDL_CONTROLLER_AXIS_LEFTX] = OtrLib::stoi(Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_LEFTX) + "_threshold"]);
        ThresholdMapping[SDL_CONTROLLER_AXIS_LEFTY] = OtrLib::stoi(Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_LEFTY) + "_threshold"]);
        ThresholdMapping[SDL_CONTROLLER_AXIS_RIGHTX] = OtrLib::stoi(Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_RIGHTX) + "_threshold"]);
        ThresholdMapping[SDL_CONTROLLER_AXIS_RIGHTY] = OtrLib::stoi(Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_RIGHTY) + "_threshold"]);
        ThresholdMapping[SDL_CONTROLLER_AXIS_TRIGGERLEFT] = OtrLib::stoi(Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_TRIGGERLEFT) + "_threshold"]);
        ThresholdMapping[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = OtrLib::stoi(Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) + "_threshold"]);
    }

    void SDLController::NormalizeStickAxis(int16_t wAxisValueX, int16_t wAxisValueY, int16_t wAxisThreshold) {
        uint32_t MagSquared = (uint32_t)(wAxisValueX * wAxisValueX) + (uint32_t)(wAxisValueY * wAxisValueY);
        uint32_t ThresholdSquared = wAxisThreshold * wAxisThreshold;

        if (MagSquared > ThresholdSquared) {
            wStickX = wAxisValueX / 256;
            int32_t StickY = -wAxisValueY / 256;
            wStickY = StickY == 128 ? 127 : StickY;
        } else {
            wStickX = 0;
            wStickY = 0;
        }
    }

    void SDLController::Read(OSContPad* pad) {
        std::string ConfSection = GetBindingConfSection();
        std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
        OTRConfigFile& Conf = *pConf.get();

        SDL_GameControllerUpdate();

        // If the controller is disconnected, close it.
        if (Cont != nullptr && !SDL_GameControllerGetAttached(Cont)) {
            Close();
        }

        // Attempt to load the controller if it's not loaded
        if (Cont == nullptr) {
            // If we failed to load the controller, don't process it.
            if (!Open()) {
                return;
            }
        }

        for (int32_t i = SDL_CONTROLLER_BUTTON_A; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
            if (ButtonMapping.contains(i)) {
                if (SDL_GameControllerGetButton(Cont, (SDL_GameControllerButton)i)) {
                    dwPressedButtons |= ButtonMapping[i];
                } else {
                    dwPressedButtons &= ~ButtonMapping[i];
                }
            }
        }

        SDL_GameControllerAxis StickAxisX = SDL_CONTROLLER_AXIS_INVALID;
        SDL_GameControllerAxis StickAxisY = SDL_CONTROLLER_AXIS_INVALID;
        int32_t StickDeadzone = 0;

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            auto Axis = (SDL_GameControllerAxis)i;
            auto PosScancode = i + AXIS_SCANCODE_BIT;
            auto NegScancode = -PosScancode;
            auto AxisThreshold = ThresholdMapping[i];
            auto PosButton = ButtonMapping[PosScancode];
            auto NegButton = ButtonMapping[NegScancode];
            auto AxisValue = SDL_GameControllerGetAxis(Cont, Axis);

            // If the axis is NOT mapped to the control stick.
            if (!(
                PosButton == BTN_STICKLEFT || PosButton == BTN_STICKRIGHT ||
                PosButton == BTN_STICKUP || PosButton == BTN_STICKDOWN ||
                NegButton == BTN_STICKLEFT || NegButton == BTN_STICKRIGHT |
                NegButton == BTN_STICKUP || NegButton == BTN_STICKDOWN)) {
                if (AxisValue > AxisThreshold) {
                    dwPressedButtons |= PosButton;
                    dwPressedButtons &= ~NegButton;
                }
                else if (AxisValue < -AxisThreshold) {
                    dwPressedButtons &= ~PosButton;
                    dwPressedButtons |= NegButton;
                }
                else {
                    dwPressedButtons &= ~PosButton;
                    dwPressedButtons &= ~NegButton;
                }
            } else {
                if (PosButton == BTN_STICKLEFT || PosButton == BTN_STICKRIGHT) {
                    if (StickAxisX != SDL_CONTROLLER_AXIS_INVALID && StickAxisX != Axis) {
                        SPDLOG_TRACE("Invalid PosStickX configured. Neg was {} and Pos is {}", StickAxisX, Axis);
                    }

                    if (StickDeadzone != 0 && StickDeadzone != AxisThreshold) {
                        SPDLOG_TRACE("Invalid Deadzone configured. Up/Down was {} and Left/Right is {}", StickDeadzone, AxisThreshold);
                    }

                    StickDeadzone = AxisThreshold;
                    StickAxisX = Axis;
                }

                if (PosButton == BTN_STICKUP || PosButton == BTN_STICKDOWN) {
                    if (StickAxisY != SDL_CONTROLLER_AXIS_INVALID && StickAxisY != Axis) {
                        SPDLOG_TRACE("Invalid PosStickY configured. Neg was {} and Pos is {}", StickAxisY, Axis);
                    }

                    if (StickDeadzone != 0 && StickDeadzone != AxisThreshold) {
                        SPDLOG_TRACE("Invalid Deadzone configured. Left/Right was {} and Up/Down is {}", StickDeadzone, AxisThreshold);
                    }

                    StickDeadzone = AxisThreshold;
                    StickAxisY = Axis;
                }

                if (NegButton == BTN_STICKLEFT || NegButton == BTN_STICKRIGHT) {
                    if (StickAxisX != SDL_CONTROLLER_AXIS_INVALID && StickAxisX != Axis) {
                        SPDLOG_TRACE("Invalid NegStickX configured. Pos was {} and Neg is {}", StickAxisX, Axis);
                    }

                    if (StickDeadzone != 0 && StickDeadzone != AxisThreshold) {
                        SPDLOG_TRACE("Invalid Deadzone configured. Left/Right was {} and Up/Down is {}", StickDeadzone, AxisThreshold);
                    }

                    StickDeadzone = AxisThreshold;
                    StickAxisX = Axis;
                }

                if (NegButton == BTN_STICKUP || NegButton == BTN_STICKDOWN) {
                    if (StickAxisY != SDL_CONTROLLER_AXIS_INVALID && StickAxisY != Axis) {
                        SPDLOG_TRACE("Invalid NegStickY configured. Pos was {} and Neg is {}", StickAxisY, Axis);
                    }

                    if (StickDeadzone != 0 && StickDeadzone != AxisThreshold) {
                        SPDLOG_TRACE("Invalid Deadzone misconfigured. Left/Right was {} and Up/Down is {}", StickDeadzone, AxisThreshold);
                    }

                    StickDeadzone = AxisThreshold;
                    StickAxisY = Axis;
                }
            }
        }

        if (StickAxisX != SDL_CONTROLLER_AXIS_INVALID && StickAxisY != SDL_CONTROLLER_AXIS_INVALID) {
            auto AxisValueX = SDL_GameControllerGetAxis(Cont, StickAxisX);
            auto AxisValueY = SDL_GameControllerGetAxis(Cont, StickAxisY);
            NormalizeStickAxis(AxisValueX, AxisValueY, StickDeadzone);
        }
            
        pad->button = dwPressedButtons & 0xFFFF;

        if (dwPressedButtons & BTN_STICKLEFT) {
            pad->stick_x = -128;
        } else if (dwPressedButtons & BTN_STICKRIGHT) {
            pad->stick_x = 127;
        } else {
            pad->stick_x = wStickX;
        }

        if (dwPressedButtons & BTN_STICKDOWN) {
            pad->stick_y = -128;
        } else if (dwPressedButtons & BTN_STICKUP) {
            pad->stick_y = 127;
        } else {
            pad->stick_y = wStickY;
        }
	}

    void SDLController::CreateDefaultBinding(std::string ContGuid) {
        std::string ConfSection = GetBindingConfSection();
        std::shared_ptr<OTRConfigFile> pConf = OTRContext::GetInstance()->GetConfig();
        OTRConfigFile& Conf = *pConf.get();

        Conf[ConfSection][STR(BTN_CRIGHT)] = std::to_string((SDL_CONTROLLER_AXIS_RIGHTX + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_CLEFT)] = std::to_string(-(SDL_CONTROLLER_AXIS_RIGHTX + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_CDOWN)] = std::to_string((SDL_CONTROLLER_AXIS_RIGHTY + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_CUP)] = std::to_string(-(SDL_CONTROLLER_AXIS_RIGHTY + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_R)] = std::to_string((SDL_CONTROLLER_AXIS_TRIGGERRIGHT + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_L)] = std::to_string(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        Conf[ConfSection][STR(BTN_DRIGHT)] = std::to_string(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        Conf[ConfSection][STR(BTN_DLEFT)] = std::to_string(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        Conf[ConfSection][STR(BTN_DDOWN)] = std::to_string(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        Conf[ConfSection][STR(BTN_DUP)] = std::to_string(SDL_CONTROLLER_BUTTON_DPAD_UP);
        Conf[ConfSection][STR(BTN_START)] = std::to_string(SDL_CONTROLLER_BUTTON_START);
        Conf[ConfSection][STR(BTN_Z)] = std::to_string((SDL_CONTROLLER_AXIS_TRIGGERLEFT + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_B)] = std::to_string(SDL_CONTROLLER_BUTTON_X);
        Conf[ConfSection][STR(BTN_A)] = std::to_string(SDL_CONTROLLER_BUTTON_A);
        Conf[ConfSection][STR(BTN_STICKRIGHT)] = std::to_string((SDL_CONTROLLER_AXIS_LEFTX + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_STICKLEFT)] = std::to_string(-(SDL_CONTROLLER_AXIS_LEFTX + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_STICKDOWN)] = std::to_string((SDL_CONTROLLER_AXIS_LEFTY + AXIS_SCANCODE_BIT));
        Conf[ConfSection][STR(BTN_STICKUP)] = std::to_string(-(SDL_CONTROLLER_AXIS_LEFTY + AXIS_SCANCODE_BIT));

        Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_LEFTX) + "_threshold"] = std::to_string(4960);
        Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_LEFTY) + "_threshold"] = std::to_string(4960);
        Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_RIGHTX) + "_threshold"] = std::to_string(0x4000);
        Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_RIGHTY) + "_threshold"] = std::to_string(0x4000);
        Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_TRIGGERLEFT) + "_threshold"] = std::to_string(0x1E00);
        Conf[ConfSection][STR(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) + "_threshold"] = std::to_string(0x1E00);
    }

    void SDLController::SetButtonMapping(std::string szButtonName, int32_t dwScancode) {
        if (guid.compare(INVALID_SDL_CONTROLLER_GUID)) {
            return;
        }

        OTRController::SetButtonMapping(szButtonName, dwScancode);
    }

	std::string SDLController::GetControllerType() {
		return "SDL";
	}

    std::string SDLController::GetConfSection() {
        return GetControllerType() + " CONTROLLER " + std::to_string(GetControllerNumber() + 1);
    }

    std::string SDLController::GetBindingConfSection() {
        return GetControllerType() + " CONTROLLER BINDING " + guid;
    }
}