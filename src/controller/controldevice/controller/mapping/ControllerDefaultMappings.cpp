#include "ControllerDefaultMappings.h"
#include "libultraship/libultra/controller.h"

namespace Ship {
ControllerDefaultMappings::ControllerDefaultMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardButtonMappings) {
    SetDefaultKeyboardButtonMappings(defaultKeyboardButtonMappings);
}

ControllerDefaultMappings::ControllerDefaultMappings()
    : ControllerDefaultMappings(std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>>()) {
}

ControllerDefaultMappings::~ControllerDefaultMappings() {
}

std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>>
ControllerDefaultMappings::GetDefaultKeyboardButtonMappings() {
    return mDefaultKeyboardButtonMappings;
}

void ControllerDefaultMappings::SetDefaultKeyboardButtonMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardButtonMappings) {
    if (!defaultKeyboardButtonMappings.empty()) {
        mDefaultKeyboardButtonMappings = defaultKeyboardButtonMappings;
        return;
    }

    mDefaultKeyboardButtonMappings[BTN_A] = { KbScancode::LUS_KB_X };
    mDefaultKeyboardButtonMappings[BTN_B] = { KbScancode::LUS_KB_C };
    mDefaultKeyboardButtonMappings[BTN_L] = { KbScancode::LUS_KB_E };
    mDefaultKeyboardButtonMappings[BTN_R] = { KbScancode::LUS_KB_R };
    mDefaultKeyboardButtonMappings[BTN_Z] = { KbScancode::LUS_KB_Z };
    mDefaultKeyboardButtonMappings[BTN_START] = { KbScancode::LUS_KB_SPACE };
    mDefaultKeyboardButtonMappings[BTN_CUP] = { KbScancode::LUS_KB_ARROWKEY_UP };
    mDefaultKeyboardButtonMappings[BTN_CDOWN] = { KbScancode::LUS_KB_ARROWKEY_DOWN };
    mDefaultKeyboardButtonMappings[BTN_CLEFT] = { KbScancode::LUS_KB_ARROWKEY_LEFT };
    mDefaultKeyboardButtonMappings[BTN_CRIGHT] = { KbScancode::LUS_KB_ARROWKEY_RIGHT };
    mDefaultKeyboardButtonMappings[BTN_DUP] = { KbScancode::LUS_KB_T };
    mDefaultKeyboardButtonMappings[BTN_DDOWN] = { KbScancode::LUS_KB_G };
    mDefaultKeyboardButtonMappings[BTN_DLEFT] = { KbScancode::LUS_KB_F };
    mDefaultKeyboardButtonMappings[BTN_DRIGHT] = { KbScancode::LUS_KB_H };
}
} // namespace Ship
