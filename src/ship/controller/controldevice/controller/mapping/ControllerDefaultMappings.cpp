#include "ship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"

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
    mDefaultKeyboardKeyToButtonMappings = defaultKeyboardKeyToButtonMappings;
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
    mDefaultSDLButtonToButtonMappings = defaultSDLButtonToButtonMappings;
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
    mDefaultSDLAxisDirectionToButtonMappings = defaultSDLAxisDirectionToButtonMappings;
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
