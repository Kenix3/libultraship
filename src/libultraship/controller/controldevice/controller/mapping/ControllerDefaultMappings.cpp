#include "libultraship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"
#include "libultraship/libultra/controller.h"

namespace LUS {
ControllerDefaultMappings::ControllerDefaultMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<Ship::KbScancode>> defaultKeyboardKeyToButtonMappings,
    std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, Ship::KbScancode>>>
        defaultKeyboardKeyToAxisDirectionMappings,
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GamepadButton>> defaultSDLButtonToButtonMappings,
    std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, SDL_GamepadButton>>>
        defaultSDLButtonToAxisDirectionMappings,
    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GamepadAxis, int32_t>>>
        defaultSDLAxisDirectionToButtonMappings,
    std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, std::pair<SDL_GamepadAxis, int32_t>>>>
        defaultSDLAxisDirectionToAxisDirectionMappings)
    : Ship::ControllerDefaultMappings(defaultKeyboardKeyToButtonMappings, defaultKeyboardKeyToAxisDirectionMappings,
                                      defaultSDLButtonToButtonMappings, defaultSDLButtonToAxisDirectionMappings,
                                      defaultSDLAxisDirectionToButtonMappings,
                                      defaultSDLAxisDirectionToAxisDirectionMappings) {
    SetDefaultKeyboardKeyToButtonMappings(defaultKeyboardKeyToButtonMappings);
    SetDefaultSDLButtonToButtonMappings(defaultSDLButtonToButtonMappings);
    SetDefaultSDLAxisDirectionToButtonMappings(defaultSDLAxisDirectionToButtonMappings);
}

ControllerDefaultMappings::ControllerDefaultMappings()
    : ControllerDefaultMappings(
          std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<Ship::KbScancode>>(),
          std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, Ship::KbScancode>>>(),
          std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GamepadButton>>(),
          std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, SDL_GamepadButton>>>(),
          std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GamepadAxis, int32_t>>>(),
          std::unordered_map<Ship::StickIndex,
                             std::vector<std::pair<Ship::Direction, std::pair<SDL_GamepadAxis, int32_t>>>>()) {
}

ControllerDefaultMappings::~ControllerDefaultMappings() {
}

void ControllerDefaultMappings::SetDefaultKeyboardKeyToButtonMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<Ship::KbScancode>> defaultKeyboardKeyToButtonMappings) {
    if (!defaultKeyboardKeyToButtonMappings.empty()) {
        Ship::ControllerDefaultMappings::SetDefaultKeyboardKeyToButtonMappings(defaultKeyboardKeyToButtonMappings);
        return;
    }

    Ship::ControllerDefaultMappings::SetDefaultKeyboardKeyToButtonMappings({
        { BTN_A, { Ship::KbScancode::LUS_KB_X } },
        { BTN_B, { Ship::KbScancode::LUS_KB_C } },
        { BTN_L, { Ship::KbScancode::LUS_KB_E } },
        { BTN_R, { Ship::KbScancode::LUS_KB_R } },
        { BTN_Z, { Ship::KbScancode::LUS_KB_Z } },
        { BTN_START, { Ship::KbScancode::LUS_KB_SPACE } },
        { BTN_CUP, { Ship::KbScancode::LUS_KB_ARROWKEY_UP } },
        { BTN_CDOWN, { Ship::KbScancode::LUS_KB_ARROWKEY_DOWN } },
        { BTN_CLEFT, { Ship::KbScancode::LUS_KB_ARROWKEY_LEFT } },
        { BTN_CRIGHT, { Ship::KbScancode::LUS_KB_ARROWKEY_RIGHT } },
        { BTN_DUP, { Ship::KbScancode::LUS_KB_T } },
        { BTN_DDOWN, { Ship::KbScancode::LUS_KB_G } },
        { BTN_DLEFT, { Ship::KbScancode::LUS_KB_F } },
        { BTN_DRIGHT, { Ship::KbScancode::LUS_KB_H } },
    });
}

void ControllerDefaultMappings::SetDefaultSDLButtonToButtonMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GamepadButton>> defaultSDLButtonToButtonMappings) {
    if (!defaultSDLButtonToButtonMappings.empty()) {
        Ship::ControllerDefaultMappings::SetDefaultSDLButtonToButtonMappings(defaultSDLButtonToButtonMappings);
        return;
    }

    Ship::ControllerDefaultMappings::SetDefaultSDLButtonToButtonMappings({
        { BTN_A, { SDL_GAMEPAD_BUTTON_SOUTH } },
        { BTN_B, { SDL_GAMEPAD_BUTTON_EAST } },
        { BTN_L, { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER } },
        { BTN_START, { SDL_GAMEPAD_BUTTON_START } },
        { BTN_DUP, { SDL_GAMEPAD_BUTTON_DPAD_UP } },
        { BTN_DDOWN, { SDL_GAMEPAD_BUTTON_DPAD_DOWN } },
        { BTN_DLEFT, { SDL_GAMEPAD_BUTTON_DPAD_LEFT } },
        { BTN_DRIGHT, { SDL_GAMEPAD_BUTTON_DPAD_RIGHT } },
    });
}

void ControllerDefaultMappings::SetDefaultSDLAxisDirectionToButtonMappings(
    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GamepadAxis, int32_t>>>
        defaultSDLAxisDirectionToButtonMappings) {
    if (!defaultSDLAxisDirectionToButtonMappings.empty()) {
        Ship::ControllerDefaultMappings::SetDefaultSDLAxisDirectionToButtonMappings(
            defaultSDLAxisDirectionToButtonMappings);
        return;
    }

    Ship::ControllerDefaultMappings::SetDefaultSDLAxisDirectionToButtonMappings({
        { BTN_R, { { SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 1 } } },
        { BTN_Z, { { SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 1 } } },
        { BTN_CUP, { { SDL_GAMEPAD_AXIS_RIGHTY, -1 } } },
        { BTN_CDOWN, { { SDL_GAMEPAD_AXIS_RIGHTY, 1 } } },
        { BTN_CLEFT, { { SDL_GAMEPAD_AXIS_RIGHTX, -1 } } },
        { BTN_CRIGHT, { { SDL_GAMEPAD_AXIS_RIGHTX, 1 } } },
    });
}
} // namespace LUS
