#include "SDLAxisDirectionToAnyMapping.h"

#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"

namespace LUS {
SDLAxisDirectionToAnyMapping::SDLAxisDirectionToAnyMapping(LUSDeviceIndex lusDeviceIndex, int32_t sdlControllerAxis,
                                                           int32_t axisDirection)
    : ControllerInputMapping(lusDeviceIndex), SDLMapping(lusDeviceIndex) {
    mControllerAxis = static_cast<SDL_GameControllerAxis>(sdlControllerAxis);
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}

SDLAxisDirectionToAnyMapping::~SDLAxisDirectionToAnyMapping() {
}

std::string SDLAxisDirectionToAnyMapping::GetPhysicalInputName() {
    switch (mControllerAxis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            return StringHelper::Sprintf(UsesGameCubeLayout() ? "Analog Stick %s" : "Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_LEFTY:
            return StringHelper::Sprintf(UsesGameCubeLayout() ? "Analog Stick %s" : "Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_AXIS_RIGHTX:
            return StringHelper::Sprintf(UsesGameCubeLayout() ? "C Stick %s" : "Right Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_RIGHTY:
            return StringHelper::Sprintf(UsesGameCubeLayout() ? "C Stick %s" : "Right Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            if (UsesPlaystationLayout()) {
                return "L2";
            }
            if (UsesSwitchLayout()) {
                return "LR";
            }
            if (UsesXboxLayout()) {
                return "LT";
            }
            if (UsesGameCubeLayout()) {
                return "L";
            }
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            if (UsesPlaystationLayout()) {
                return "R2";
            }
            if (UsesSwitchLayout()) {
                return "ZR";
            }
            if (UsesXboxLayout()) {
                return "RT";
            }
            if (UsesGameCubeLayout()) {
                return "R";
            }
            break;
    }

    return StringHelper::Sprintf("Axis %d %s", mControllerAxis,
                                 mAxisDirection == NEGATIVE ? ICON_FA_MINUS : ICON_FA_PLUS);
}

std::string SDLAxisDirectionToAnyMapping::GetPhysicalDeviceName() {
    return GetSDLDeviceName();
}

bool SDLAxisDirectionToAnyMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}

bool SDLAxisDirectionToAnyMapping::AxisIsTrigger() {
    return mControllerAxis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || mControllerAxis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
}

bool SDLAxisDirectionToAnyMapping::AxisIsStick() {
    return mControllerAxis == SDL_CONTROLLER_AXIS_LEFTX || mControllerAxis == SDL_CONTROLLER_AXIS_LEFTY ||
           mControllerAxis == SDL_CONTROLLER_AXIS_RIGHTX || mControllerAxis == SDL_CONTROLLER_AXIS_RIGHTY;
}
} // namespace LUS
