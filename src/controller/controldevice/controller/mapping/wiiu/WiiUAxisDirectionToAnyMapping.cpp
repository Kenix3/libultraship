#ifdef __WIIU__
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
            return StringHelper::Sprintf("Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_LEFTY:
            return StringHelper::Sprintf("Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case SDL_CONTROLLER_AXIS_RIGHTX:
            return StringHelper::Sprintf("Right Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case SDL_CONTROLLER_AXIS_RIGHTY:
            return StringHelper::Sprintf("Right Stick %s",
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
} // namespace LUS
#endif
