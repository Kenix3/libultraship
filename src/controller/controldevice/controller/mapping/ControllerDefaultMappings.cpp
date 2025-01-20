#include "ControllerDefaultMappings.h"
#include "libultraship/libultra/controller.h"

namespace Ship {
ControllerDefaultMappings::ControllerDefaultMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings) {
    SetDefaultKeyboardKeyToButtonMappings(defaultKeyboardKeyToButtonMappings);
}

ControllerDefaultMappings::ControllerDefaultMappings()
    : ControllerDefaultMappings(std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>>()) {
}

ControllerDefaultMappings::~ControllerDefaultMappings() {
}

std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>>
ControllerDefaultMappings::GetDefaultKeyboardKeyToButtonMappings() {
    return mDefaultKeyboardKeyToButtonMappings;
}

void ControllerDefaultMappings::SetDefaultKeyboardKeyToButtonMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings) {
    if (!defaultKeyboardKeyToButtonMappings.empty()) {
        mDefaultKeyboardKeyToButtonMappings = defaultKeyboardKeyToButtonMappings;
        return;
    }

    mDefaultKeyboardKeyToButtonMappings[BTN_A] = { KbScancode::LUS_KB_X };
    mDefaultKeyboardKeyToButtonMappings[BTN_B] = { KbScancode::LUS_KB_C };
    mDefaultKeyboardKeyToButtonMappings[BTN_L] = { KbScancode::LUS_KB_E };
    mDefaultKeyboardKeyToButtonMappings[BTN_R] = { KbScancode::LUS_KB_R };
    mDefaultKeyboardKeyToButtonMappings[BTN_Z] = { KbScancode::LUS_KB_Z };
    mDefaultKeyboardKeyToButtonMappings[BTN_START] = { KbScancode::LUS_KB_SPACE };
    mDefaultKeyboardKeyToButtonMappings[BTN_CUP] = { KbScancode::LUS_KB_ARROWKEY_UP };
    mDefaultKeyboardKeyToButtonMappings[BTN_CDOWN] = { KbScancode::LUS_KB_ARROWKEY_DOWN };
    mDefaultKeyboardKeyToButtonMappings[BTN_CLEFT] = { KbScancode::LUS_KB_ARROWKEY_LEFT };
    mDefaultKeyboardKeyToButtonMappings[BTN_CRIGHT] = { KbScancode::LUS_KB_ARROWKEY_RIGHT };
    mDefaultKeyboardKeyToButtonMappings[BTN_DUP] = { KbScancode::LUS_KB_T };
    mDefaultKeyboardKeyToButtonMappings[BTN_DDOWN] = { KbScancode::LUS_KB_G };
    mDefaultKeyboardKeyToButtonMappings[BTN_DLEFT] = { KbScancode::LUS_KB_F };
    mDefaultKeyboardKeyToButtonMappings[BTN_DRIGHT] = { KbScancode::LUS_KB_H };
}
} // namespace Ship
