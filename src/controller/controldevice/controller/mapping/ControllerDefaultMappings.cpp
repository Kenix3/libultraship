#include "ControllerDefaultMappings.h"
#include "libultraship/libultra/controller.h"

namespace Ship {
ControllerDefaultMappings::ControllerDefaultMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings,
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
        defaultKeyboardKeyToAxisDirectionMappings,
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
        defaultSDLButtonToButtonMappings,
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
        defaultSDLButtonToAxisDirectionMappings,
    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
        defaultSDLAxisDirectionToButtonMappings,
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
        defaultSDLAxisDirectionToAxisDirectionMappings) {
    SetDefaultKeyboardKeyToButtonMappings(defaultKeyboardKeyToButtonMappings);
    SetDefaultKeyboardKeyToAxisDirectionMappings(defaultKeyboardKeyToAxisDirectionMappings);

    SetDefaultSDLButtonToButtonMappings(defaultSDLButtonToButtonMappings);
    SetDefaultSDLButtonToAxisDirectionMappings(defaultSDLButtonToAxisDirectionMappings);

    SetDefaultSDLAxisDirectionToButtonMappings(defaultSDLAxisDirectionToButtonMappings);
    SetDefaultSDLAxisDirectionToAxisDirectionMappings(defaultSDLAxisDirectionToAxisDirectionMappings);
}

ControllerDefaultMappings::ControllerDefaultMappings()
    : ControllerDefaultMappings(
          std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>>(),
          std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>(),
          std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>(),
          std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>(),
          std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>(),
          std::unordered_map<StickIndex,
                             std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>()) {
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

std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
ControllerDefaultMappings::GetDefaultKeyboardKeyToAxisDirectionMappings() {
    return mDefaultKeyboardKeyToAxisDirectionMappings;
}

void ControllerDefaultMappings::SetDefaultKeyboardKeyToAxisDirectionMappings(
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
        defaultKeyboardKeyToAxisDirectionMappings) {
    if (!defaultKeyboardKeyToAxisDirectionMappings.empty()) {
        mDefaultKeyboardKeyToAxisDirectionMappings = defaultKeyboardKeyToAxisDirectionMappings;
        return;
    }

    mDefaultKeyboardKeyToAxisDirectionMappings[LEFT_STICK] = { { LEFT, KbScancode::LUS_KB_A },
                                                               { RIGHT, KbScancode::LUS_KB_D },
                                                               { UP, KbScancode::LUS_KB_W },
                                                               { DOWN, KbScancode::LUS_KB_S } };
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

std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
ControllerDefaultMappings::GetDefaultSDLButtonToAxisDirectionMappings() {
    return mDefaultSDLButtonToAxisDirectionMappings;
}

void ControllerDefaultMappings::SetDefaultSDLButtonToAxisDirectionMappings(
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
        defaultSDLButtonToAxisDirectionMappings) {
    if (!defaultSDLButtonToAxisDirectionMappings.empty()) {
        mDefaultSDLButtonToAxisDirectionMappings = defaultSDLButtonToAxisDirectionMappings;
        return;
    }
}

std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
ControllerDefaultMappings::GetDefaultSDLAxisDirectionToButtonMappings() {
    return mDefaultSDLAxisDirectionToButtonMappings;
}

void ControllerDefaultMappings::SetDefaultSDLAxisDirectionToButtonMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
        defaultSDLAxisDirectionToButtonMappings) {
    if (!defaultSDLAxisDirectionToButtonMappings.empty()) {
        mDefaultSDLAxisDirectionToButtonMappings = defaultSDLAxisDirectionToButtonMappings;
        return;
    }

    mDefaultSDLAxisDirectionToButtonMappings[BTN_R] = { { SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1 } };
    mDefaultSDLAxisDirectionToButtonMappings[BTN_Z] = { { SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1 } };
    mDefaultSDLAxisDirectionToButtonMappings[BTN_CUP] = { { SDL_CONTROLLER_AXIS_RIGHTY, -1 } };
    mDefaultSDLAxisDirectionToButtonMappings[BTN_CDOWN] = { { SDL_CONTROLLER_AXIS_RIGHTY, 1 } };
    mDefaultSDLAxisDirectionToButtonMappings[BTN_CLEFT] = { { SDL_CONTROLLER_AXIS_RIGHTX, -1 } };
    mDefaultSDLAxisDirectionToButtonMappings[BTN_CRIGHT] = { { SDL_CONTROLLER_AXIS_RIGHTX, 1 } };
}

std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
ControllerDefaultMappings::GetDefaultSDLAxisDirectionToAxisDirectionMappings() {
    return mDefaultSDLAxisDirectionToAxisDirectionMappings;
}

void ControllerDefaultMappings::SetDefaultSDLAxisDirectionToAxisDirectionMappings(
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
        defaultSDLAxisDirectionToAxisDirectionMappings) {
    if (!defaultSDLAxisDirectionToAxisDirectionMappings.empty()) {
        mDefaultSDLAxisDirectionToAxisDirectionMappings = defaultSDLAxisDirectionToAxisDirectionMappings;
        return;
    }

    mDefaultSDLAxisDirectionToAxisDirectionMappings[LEFT_STICK] = {
        { LEFT, { SDL_CONTROLLER_AXIS_LEFTX, -1 } },
        { RIGHT, { SDL_CONTROLLER_AXIS_LEFTX, 1 } },
        { UP, { SDL_CONTROLLER_AXIS_LEFTY, -1 } },
        { DOWN, { SDL_CONTROLLER_AXIS_LEFTY, 1 } },
    };

    mDefaultSDLAxisDirectionToAxisDirectionMappings[RIGHT_STICK] = {
        { LEFT, { SDL_CONTROLLER_AXIS_RIGHTX, -1 } },
        { RIGHT, { SDL_CONTROLLER_AXIS_RIGHTX, 1 } },
        { UP, { SDL_CONTROLLER_AXIS_RIGHTY, -1 } },
        { DOWN, { SDL_CONTROLLER_AXIS_RIGHTY, 1 } },
    };
}

} // namespace Ship
