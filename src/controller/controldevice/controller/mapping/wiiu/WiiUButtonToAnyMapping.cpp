#ifdef __WIIU__
#include "WiiUButtonToAnyMapping.h"

#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"

namespace LUS {
WiiUButtonToAnyMapping::WiiUButtonToAnyMapping(LUSDeviceIndex lusDeviceIndex, bool isNunchuk, bool isClassic,
                                               uint32_t wiiuControllerButton)
    : ControllerInputMapping(lusDeviceIndex), WiiUMapping(lusDeviceIndex), mIsNunchukButton(isNunchuk),
      mControllerButton(wiiuControllerButton) {
}

WiiUButtonToAnyMapping::~WiiUButtonToAnyMapping() {
}

std::string WiiUButtonToAnyMapping::GetGamepadButtonName() {
    switch (mControllerButton) {
        case VPAD_BUTTON_A:
            return "A";
        case VPAD_BUTTON_B:
            return "B";
        case VPAD_BUTTON_X:
            return "X";
        case VPAD_BUTTON_Y:
            return "Y";
        case VPAD_BUTTON_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case VPAD_BUTTON_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case VPAD_BUTTON_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case VPAD_BUTTON_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case VPAD_BUTTON_ZL:
            return "ZL";
        case VPAD_BUTTON_ZR:
            return "ZR";
        case VPAD_BUTTON_L:
            return "L";
        case VPAD_BUTTON_R:
            return "R";
        case VPAD_BUTTON_PLUS:
            return StringHelper::Sprintf("%s", ICON_FA_PLUS);
        case VPAD_BUTTON_MINUS:
            return StringHelper::Sprintf("%s", ICON_FA_MINUS);
        case VPAD_BUTTON_HOME:
            return StringHelper::Sprintf("%s", ICON_FA_HOME);
        case VPAD_BUTTON_SYNC:
            return "Sync";
        case VPAD_BUTTON_STICK_R:
            return "Right Stick Press"; // it seems the official term is just long for these
        case VPAD_BUTTON_STICK_L:
            return "Left Stick Press"; // it seems the official term is just long for these
        case VPAD_BUTTON_TV:
            return "TV";
        case VPAD_STICK_R_EMULATION_LEFT:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_LEFT);
        case VPAD_STICK_R_EMULATION_RIGHT:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_RIGHT);
        case VPAD_STICK_R_EMULATION_UP:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_UP);
        case VPAD_STICK_R_EMULATION_DOWN:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_DOWN);
        case VPAD_STICK_L_EMULATION_LEFT:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_LEFT);
        case VPAD_STICK_L_EMULATION_RIGHT:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_RIGHT);
        case VPAD_STICK_L_EMULATION_UP:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_UP);
        case VPAD_STICK_L_EMULATION_DOWN:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_DOWN);
        default:
            return "Unknown";
    }
}

std::string WiiUButtonToAnyMapping::GetWiiRemoteButtonName() {
    switch (mControllerButton) {
        case WPAD_BUTTON_LEFT:
            return StringHelper::Sprintf("D-Pad %s (Wii Remote)", ICON_FA_ARROW_LEFT);
        case WPAD_BUTTON_RIGHT:
            return StringHelper::Sprintf("D-Pad %s (Wii Remote)", ICON_FA_ARROW_RIGHT);
        case WPAD_BUTTON_DOWN:
            return StringHelper::Sprintf("D-Pad %s (Wii Remote)", ICON_FA_ARROW_DOWN);
        case WPAD_BUTTON_UP:
            return StringHelper::Sprintf("D-Pad %s (Wii Remote)", ICON_FA_ARROW_UP);
        case WPAD_BUTTON_PLUS:
            return StringHelper::Sprintf("%s", ICON_FA_PLUS);
        case WPAD_BUTTON_2:
            return "2";
        case WPAD_BUTTON_1:
            return "1";
        case WPAD_BUTTON_B:
            return "B (Wii Remote)";
        case WPAD_BUTTON_A:
            return "A (Wii Remote)";
        case WPAD_BUTTON_MINUS:
            return StringHelper::Sprintf("%s", ICON_FA_MINUS);
        case WPAD_BUTTON_Z:
            return "Z";
        case WPAD_BUTTON_C:
            return "C";
        case WPAD_BUTTON_HOME:
            return StringHelper::Sprintf("%s", ICON_FA_HOME);
        default:
            return "Unknown";
    }
}

std::string WiiUButtonToAnyMapping::GetNunchukButtonName() {
    switch (mControllerButton) {
        case WPAD_NUNCHUK_STICK_EMULATION_LEFT:
            return StringHelper::Sprintf("Nunchuk %s", ICON_FA_ARROW_LEFT);
        case WPAD_NUNCHUK_STICK_EMULATION_RIGHT:
            return StringHelper::Sprintf("Nunchuk %s", ICON_FA_ARROW_RIGHT);
        case WPAD_NUNCHUK_STICK_EMULATION_DOWN:
            return StringHelper::Sprintf("Nunchuk %s", ICON_FA_ARROW_DOWN);
        case WPAD_NUNCHUK_STICK_EMULATION_UP:
            return StringHelper::Sprintf("Nunchuk %s", ICON_FA_ARROW_UP);
        case WPAD_NUNCHUK_BUTTON_Z:
            return "Z";
        case WPAD_NUNCHUK_BUTTON_C:
            return "C";
        default:
            return "Unknown";
    }
}

std::string WiiUButtonToAnyMapping::GetClassicControllerButtonName() {
    switch (mControllerButton) {
        case WPAD_CLASSIC_BUTTON_UP:
            return StringHelper::Sprintf("D-Pad %s (Classic)", ICON_FA_ARROW_UP);
        case WPAD_CLASSIC_BUTTON_LEFT:
            return StringHelper::Sprintf("D-Pad %s (Classic)", ICON_FA_ARROW_LEFT);
        case WPAD_CLASSIC_BUTTON_ZR:
            return "ZR";
        case WPAD_CLASSIC_BUTTON_X:
            return "X";
        case WPAD_CLASSIC_BUTTON_A:
            return "A (Classic)";
        case WPAD_CLASSIC_BUTTON_Y:
            return "Y";
        case WPAD_CLASSIC_BUTTON_B:
            return "B (Classic)";
        case WPAD_CLASSIC_BUTTON_ZL:
            return "ZL";
        case WPAD_CLASSIC_BUTTON_R:
            return "R";
        case WPAD_CLASSIC_BUTTON_PLUS:
            return StringHelper::Sprintf("%s", ICON_FA_PLUS);
        case WPAD_CLASSIC_BUTTON_HOME:
            return StringHelper::Sprintf("%s", ICON_FA_HOME);
        case WPAD_CLASSIC_BUTTON_MINUS:
            return StringHelper::Sprintf("%s", ICON_FA_MINUS);
        case WPAD_CLASSIC_BUTTON_L:
            return "L";
        case WPAD_CLASSIC_BUTTON_DOWN:
            return StringHelper::Sprintf("Classic D-Pad %s", ICON_FA_ARROW_DOWN);
        case WPAD_CLASSIC_BUTTON_RIGHT:
            return StringHelper::Sprintf("Classic D-Pad %s", ICON_FA_ARROW_RIGHT);
        case WPAD_CLASSIC_STICK_L_EMULATION_LEFT:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_LEFT);
        case WPAD_CLASSIC_STICK_L_EMULATION_RIGHT:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_RIGHT);
        case WPAD_CLASSIC_STICK_L_EMULATION_DOWN:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_DOWN);
        case WPAD_CLASSIC_STICK_L_EMULATION_UP:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_UP);
        case WPAD_CLASSIC_STICK_R_EMULATION_LEFT:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_LEFT);
        case WPAD_CLASSIC_STICK_R_EMULATION_RIGHT:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_RIGHT);
        case WPAD_CLASSIC_STICK_R_EMULATION_DOWN:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_DOWN);
        case WPAD_CLASSIC_STICK_R_EMULATION_UP:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_UP);
        default:
            return "Unknown";
    }
}

std::string WiiUButtonToAnyMapping::GetProControllerButtonName() {
    switch (mControllerButton) {
        case WPAD_PRO_BUTTON_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case WPAD_PRO_BUTTON_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case WPAD_PRO_TRIGGER_ZR:
            return "ZR";
        case WPAD_PRO_BUTTON_X:
            return "X";
        case WPAD_PRO_BUTTON_A:
            return "A";
        case WPAD_PRO_BUTTON_Y:
            return "Y";
        case WPAD_PRO_BUTTON_B:
            return "B";
        case WPAD_PRO_TRIGGER_ZL:
            return "ZL";
        case WPAD_PRO_TRIGGER_R:
            return "R";
        case WPAD_PRO_BUTTON_PLUS:
            return StringHelper::Sprintf("%s", ICON_FA_PLUS);
        case WPAD_PRO_BUTTON_HOME:
            return StringHelper::Sprintf("%s", ICON_FA_HOME);
        case WPAD_PRO_BUTTON_MINUS:
            return StringHelper::Sprintf("%s", ICON_FA_MINUS);
        case WPAD_PRO_TRIGGER_L:
            return "L";
        case WPAD_PRO_BUTTON_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case WPAD_PRO_BUTTON_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case WPAD_PRO_BUTTON_STICK_R:
            return "Right Stick Press"; // it seems the official term is just long for these
        case WPAD_PRO_BUTTON_STICK_L:
            return "Left Stick Press"; // it seems the official term is just long for these
        case WPAD_PRO_STICK_L_EMULATION_UP:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_UP);
        case WPAD_PRO_STICK_L_EMULATION_DOWN:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_DOWN);
        case WPAD_PRO_STICK_L_EMULATION_LEFT:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_LEFT);
        case WPAD_PRO_STICK_L_EMULATION_RIGHT:
            return StringHelper::Sprintf("Left Stick %s", ICON_FA_ARROW_RIGHT);
        case WPAD_PRO_STICK_R_EMULATION_UP:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_UP);
        case WPAD_PRO_STICK_R_EMULATION_DOWN:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_DOWN);
        case WPAD_PRO_STICK_R_EMULATION_LEFT:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_LEFT);
        case WPAD_PRO_STICK_R_EMULATION_RIGHT:
            return StringHelper::Sprintf("Right Stick %s", ICON_FA_ARROW_RIGHT);
        default:
            return "Unknown";
    }
}

std::string WiiUButtonToAnyMapping::GetPhysicalInputName() {
    if (IsGamepad()) {
        return GetGamepadButtonName();
    }

    if (ExtensionType() == WPAD_EXT_PRO_CONTROLLER) {
        return GetProControllerButtonName();
    }

    if (mIsClassicControllerButton) {
        return GetClassicControllerButtonName();
    }

    if (mIsNunchukButton) {
        return GetNunchukButtonName();
    }

    return GetWiiRemoteButtonName();
}

std::string WiiUButtonToAnyMapping::GetPhysicalDeviceName() {
    return GetWiiUDeviceName();
}

bool WiiUButtonToAnyMapping::PhysicalDeviceIsConnected() {
    return WiiUDeviceIsConnected();
}

bool WiiUButtonToAnyMapping::PhysicalButtonIsPressed() {
    if (IsGamepad()) {
        VPADReadError error;
        VPADStatus* status = LUS::WiiU::GetVPADStatus(&error);
        if (status == nullptr) {
            return false;
        }

        return status->hold & mControllerButton;
    }

    KPADError error;
    KPADStatus* status = LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(GetWiiUDeviceChannel()), &error);
    if (status == nullptr || error != KPAD_ERROR_OK) {
        return false;
    }

    if (ExtensionType() == WPAD_EXT_PRO_CONTROLLER) {
        return status->pro.hold & mControllerButton;
    }

    if (mIsClassicControllerButton) {
        return status->classic.hold & mControllerButton;
    }

    if (mIsNunchukButton) {
        return status->nunchuck.hold & mControllerButton;
    }

    return status->hold & mControllerButton;
}
} // namespace LUS
#endif
