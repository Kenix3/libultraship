#include "SDLButtonToAnyMapping.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
SDLButtonToAnyMapping::SDLButtonToAnyMapping(ShipDeviceIndex shipDeviceIndex, int32_t sdlControllerButton)
    : ControllerInputMapping(shipDeviceIndex), SDLMapping(shipDeviceIndex) {
    mControllerButton = static_cast<SDL_GamepadButton>(sdlControllerButton);
}

SDLButtonToAnyMapping::~SDLButtonToAnyMapping() {
}

std::string SDLButtonToAnyMapping::GetPhysicalInputName() {
    if (UsesPlaystationLayout()) {
        return GetPlaystationButtonName();
    }

    if (UsesSwitchLayout()) {
        return GetSwitchButtonName();
    }

    if (UsesXboxLayout()) {
        return GetXboxButtonName();
    }

    if (UsesGameCubeLayout()) {
        return GetGameCubeButtonName();
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetGenericButtonName() {
    return StringHelper::Sprintf("B%d", mControllerButton);
}

std::string SDLButtonToAnyMapping::GetGameCubeButtonName() {
    switch (mControllerButton) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return "A";
        case SDL_GAMEPAD_BUTTON_EAST:
            return "B";
        case SDL_GAMEPAD_BUTTON_WEST:
            return "X";
        case SDL_GAMEPAD_BUTTON_NORTH:
            return "Y";
        case SDL_GAMEPAD_BUTTON_START:
            return "Start";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return "Z";
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        default:
            break;
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetPlaystationButtonName() {
    switch (mControllerButton) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return StringHelper::Sprintf("%s", ICON_FA_TIMES);
        case SDL_GAMEPAD_BUTTON_EAST:
            return StringHelper::Sprintf("%s", ICON_FA_CIRCLE_O);
        case SDL_GAMEPAD_BUTTON_WEST:
            return StringHelper::Sprintf("%s", ICON_FA_SQUARE_O);
        case SDL_GAMEPAD_BUTTON_NORTH:
            return "Triangle"; // imgui default font doesn't have Δ, and font-awesome 4 doesn't have a triangle
        case SDL_GAMEPAD_BUTTON_BACK:
            if (GetSDLControllerType() == SDL_GAMEPAD_TYPE_PS3) {
                return "Select";
            }
            if (GetSDLControllerType() == SDL_GAMEPAD_TYPE_PS4) {
                return "Share";
            }
            return "Create";
        case SDL_GAMEPAD_BUTTON_GUIDE:
            return "PS";
        case SDL_GAMEPAD_BUTTON_START:
            if (GetSDLControllerType() == SDL_GAMEPAD_TYPE_PS3) {
                return "Start";
            }
            if (GetSDLControllerType() == SDL_GAMEPAD_TYPE_PS4) {
                return "Options";
            }
            return StringHelper::Sprintf("%s", ICON_FA_BARS);
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return "L3"; // it seems the official term is just long for these
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return "R3"; // it seems the official term is just long for these
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return "L1";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return "R1";
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case SDL_GAMEPAD_BUTTON_MISC1:
            if (GetSDLControllerType() == SDL_GAMEPAD_TYPE_PS5) {
                return StringHelper::Sprintf("%s", ICON_FA_MICROPHONE_SLASH);
            }
            break;
        default:
            break;
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetSwitchButtonName() {
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
            return StringHelper::Sprintf("%s", ICON_FA_MINUS);
        case SDL_GAMEPAD_BUTTON_GUIDE:
            return StringHelper::Sprintf("%s", ICON_FA_HOME);
        case SDL_GAMEPAD_BUTTON_START:
            return StringHelper::Sprintf("%s", ICON_FA_PLUS);
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return "Left Stick Press"; // it seems the official term is just long for these
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return "Right Stick Press"; // it seems the official term is just long for these
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return "L";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return "R";
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case SDL_GAMEPAD_BUTTON_MISC1:
            return "Capture"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button,
                                 Amazon Luna microphone button */
        default:
            break;
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetXboxButtonName() {
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
            if (GetSDLControllerType() == SDL_GAMEPAD_TYPE_XBOX360) {
                return "Back";
            }
            return "View";
        case SDL_GAMEPAD_BUTTON_GUIDE:
            return "Xbox";
        case SDL_GAMEPAD_BUTTON_START:
            if (GetSDLControllerType() == SDL_GAMEPAD_TYPE_XBOX360) {
                return "Start";
            }
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

std::string SDLButtonToAnyMapping::GetPhysicalDeviceName() {
    return GetSDLDeviceName();
}

bool SDLButtonToAnyMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}
} // namespace Ship
