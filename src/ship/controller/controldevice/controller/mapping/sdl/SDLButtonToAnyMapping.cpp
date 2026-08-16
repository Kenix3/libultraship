#include "ship/controller/controldevice/controller/mapping/sdl/SDLButtonToAnyMapping.h"

#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"

namespace Ship {
SDLButtonToAnyMapping::SDLButtonToAnyMapping(int32_t sdlControllerButton)
    : ControllerInputMapping(PhysicalDeviceType::SDLGamepad) {
    mControllerButton = static_cast<SDL_GamepadButton>(sdlControllerButton);
}

SDLButtonToAnyMapping::~SDLButtonToAnyMapping() {
}

std::string SDLButtonToAnyMapping::GetPhysicalInputName() {
    switch (mControllerButton) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return "A";
        case SDL_GAMEPAD_BUTTON_EAST:
            return "B";
        case SDL_GAMEPAD_BUTTON_WEST:
            return "X";
        case SDL_GAMEPAD_BUTTON_NORTH:
            return "Y";
        case SDL_GAMEPAD_BUTTON_BACK:
            return "View";
        case SDL_GAMEPAD_BUTTON_GUIDE:
            return "Xbox";
        case SDL_GAMEPAD_BUTTON_START:
            return StringHelper::Sprintf("%s", ICON_FA_BARS);
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return "LS";
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return "RS";
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return "LB";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return "RB";
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case SDL_GAMEPAD_BUTTON_MISC1:
            return "Share"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button,
                               Amazon Luna microphone button */
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:
            return "P1";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:
            return "P2";
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:
            return "P3";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:
            return "P4";
        default:
            break;
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetGenericButtonName() {
    return StringHelper::Sprintf("B%d", mControllerButton);
}

std::string SDLButtonToAnyMapping::GetPhysicalDeviceName() {
    return "SDL Gamepad";
}
} // namespace Ship
