#include "ControllerDefaultMappings.h"
#include "libultraship/libultra/controller.h"

namespace Ship {
ControllerDefaultMappings::ControllerDefaultMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings,
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
        defaultSDLButtonToButtonMappings) {
    SetDefaultKeyboardKeyToButtonMappings(defaultKeyboardKeyToButtonMappings);
    SetDefaultSDLButtonToButtonMappings(defaultSDLButtonToButtonMappings);
}

ControllerDefaultMappings::ControllerDefaultMappings()
    : ControllerDefaultMappings(
          std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>>(),
          std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>()) {
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

std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
ControllerDefaultMappings::GetDefaultSDLButtonToButtonMappings() {
    return mDefaultSDLButtonToButtonMappings;
}

void ControllerDefaultMappings::SetDefaultSDLButtonToButtonMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
        defaultSDLButtonToButtonMappings) {
    if (!defaultSDLButtonToButtonMappings.empty()) {
        mDefaultSDLButtonToButtonMappings = defaultSDLButtonToButtonMappings;
        return;
    }

    mDefaultSDLButtonToButtonMappings[BTN_A] = { SDL_CONTROLLER_BUTTON_A };
    mDefaultSDLButtonToButtonMappings[BTN_B] = { SDL_CONTROLLER_BUTTON_B };
    mDefaultSDLButtonToButtonMappings[BTN_L] = { SDL_CONTROLLER_BUTTON_LEFTSHOULDER };
    mDefaultSDLButtonToButtonMappings[BTN_START] = { SDL_CONTROLLER_BUTTON_START };
    mDefaultSDLButtonToButtonMappings[BTN_DUP] = { SDL_CONTROLLER_BUTTON_DPAD_UP };
    mDefaultSDLButtonToButtonMappings[BTN_DDOWN] = { SDL_CONTROLLER_BUTTON_DPAD_DOWN };
    mDefaultSDLButtonToButtonMappings[BTN_DLEFT] = { SDL_CONTROLLER_BUTTON_DPAD_LEFT };
    mDefaultSDLButtonToButtonMappings[BTN_DRIGHT] = { SDL_CONTROLLER_BUTTON_DPAD_RIGHT };
}
} // namespace Ship
