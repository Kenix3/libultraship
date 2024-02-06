#include "SDLButtonToAnyMapping.h"

#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"

namespace LUS {
SDLButtonToAnyMapping::SDLButtonToAnyMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlControllerButton)
    : ControllerInputMapping(lusDeviceIndex), SDLMapping(lusDeviceIndex) {
    mControllerButton = static_cast<SDL_GameControllerButton>(sdlControllerButton);
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
        case SDL_CONTROLLER_BUTTON_A:
            return "A";
        case SDL_CONTROLLER_BUTTON_B:
            return "B";
        case SDL_CONTROLLER_BUTTON_X:
            return "X";
        case SDL_CONTROLLER_BUTTON_Y:
            return "Y";
        case SDL_CONTROLLER_BUTTON_START:
            return "Start";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return "Z";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetPlaystationButtonName() {
    switch (mControllerButton) {
        case SDL_CONTROLLER_BUTTON_A:
            return StringHelper::Sprintf("%s", ICON_FA_TIMES);
        case SDL_CONTROLLER_BUTTON_B:
            return StringHelper::Sprintf("%s", ICON_FA_CIRCLE_O);
        case SDL_CONTROLLER_BUTTON_X:
            return StringHelper::Sprintf("%s", ICON_FA_SQUARE_O);
        case SDL_CONTROLLER_BUTTON_Y:
            return "Triangle"; // imgui default font doesn't have Δ, and font-awesome 4 doesn't have a triangle
        case SDL_CONTROLLER_BUTTON_BACK:
            if (GetSDLControllerType() == SDL_CONTROLLER_TYPE_PS3) {
                return "Select";
            }
            if (GetSDLControllerType() == SDL_CONTROLLER_TYPE_PS4) {
                return "Share";
            }
            return "Create";
        case SDL_CONTROLLER_BUTTON_GUIDE:
            return "PS";
        case SDL_CONTROLLER_BUTTON_START:
            if (GetSDLControllerType() == SDL_CONTROLLER_TYPE_PS3) {
                return "Start";
            }
            if (GetSDLControllerType() == SDL_CONTROLLER_TYPE_PS4) {
                return "Options";
            }
            return StringHelper::Sprintf("%s", ICON_FA_BARS);
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            return "L3"; // it seems the official term is just long for these
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            return "R3"; // it seems the official term is just long for these
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return "L1";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return "R1";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_BUTTON_MISC1:
            if (GetSDLControllerType() == SDL_CONTROLLER_TYPE_PS5) {
                return StringHelper::Sprintf("%s", ICON_FA_MICROPHONE_SLASH);
            }
            break;
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetSwitchButtonName() {
    switch (mControllerButton) {
        case SDL_CONTROLLER_BUTTON_A:
            return "A";
        case SDL_CONTROLLER_BUTTON_B:
            return "B";
        case SDL_CONTROLLER_BUTTON_X:
            return "X";
        case SDL_CONTROLLER_BUTTON_Y:
            return "Y";
        case SDL_CONTROLLER_BUTTON_BACK:
            return StringHelper::Sprintf("%s", ICON_FA_MINUS);
        case SDL_CONTROLLER_BUTTON_GUIDE:
            return StringHelper::Sprintf("%s", ICON_FA_HOME);
        case SDL_CONTROLLER_BUTTON_START:
            return StringHelper::Sprintf("%s", ICON_FA_PLUS);
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            return "Left Stick Press"; // it seems the official term is just long for these
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            return "Right Stick Press"; // it seems the official term is just long for these
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return "L";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return "R";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_BUTTON_MISC1:
            return "Capture"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button,
                                 Amazon Luna microphone button */
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetXboxButtonName() {
    switch (mControllerButton) {
        case SDL_CONTROLLER_BUTTON_A:
            return "A";
        case SDL_CONTROLLER_BUTTON_B:
            return "B";
        case SDL_CONTROLLER_BUTTON_X:
            return "X";
        case SDL_CONTROLLER_BUTTON_Y:
            return "Y";
        case SDL_CONTROLLER_BUTTON_BACK:
            if (GetSDLControllerType() == SDL_CONTROLLER_TYPE_XBOX360) {
                return "Back";
            }
            return "View";
        case SDL_CONTROLLER_BUTTON_GUIDE:
            return "Xbox";
        case SDL_CONTROLLER_BUTTON_START:
            if (GetSDLControllerType() == SDL_CONTROLLER_TYPE_XBOX360) {
                return "Start";
            }
            return StringHelper::Sprintf("%s", ICON_FA_BARS);
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            return "LS";
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            return "RS";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return "LB";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return "RB";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_UP);
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_LEFT);
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return StringHelper::Sprintf("D-Pad %s", ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_BUTTON_MISC1:
            return "Share"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button,
                               Amazon Luna microphone button */
        case SDL_CONTROLLER_BUTTON_PADDLE1:
            return "P1";
        case SDL_CONTROLLER_BUTTON_PADDLE2:
            return "P2";
        case SDL_CONTROLLER_BUTTON_PADDLE3:
            return "P3";
        case SDL_CONTROLLER_BUTTON_PADDLE4:
            return "P4";
    }

    return GetGenericButtonName();
}

std::string SDLButtonToAnyMapping::GetPhysicalDeviceName() {
    return GetSDLDeviceName();
}

bool SDLButtonToAnyMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}
} // namespace LUS
