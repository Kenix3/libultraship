#include "SDLAxisDirectionToAnyMapping.h"

#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"

namespace Ship {
SDLAxisDirectionToAnyMapping::SDLAxisDirectionToAnyMapping(int32_t sdlControllerAxis, int32_t axisDirection)
    : ControllerInputMapping(PhysicalDeviceType::SDLGamepad) {
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
            return "LT";
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            return "RT";
            break;
        default:
            break;
    }

    return StringHelper::Sprintf("Axis %d %s", mControllerAxis,
                                 mAxisDirection == NEGATIVE ? ICON_FA_MINUS : ICON_FA_PLUS);
}

std::string SDLAxisDirectionToAnyMapping::GetPhysicalDeviceName() {
    return "SDL Gamepad";
}

bool SDLAxisDirectionToAnyMapping::AxisIsTrigger() {
    return mControllerAxis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || mControllerAxis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
}

bool SDLAxisDirectionToAnyMapping::AxisIsStick() {
    return mControllerAxis == SDL_CONTROLLER_AXIS_LEFTX || mControllerAxis == SDL_CONTROLLER_AXIS_LEFTY ||
           mControllerAxis == SDL_CONTROLLER_AXIS_RIGHTX || mControllerAxis == SDL_CONTROLLER_AXIS_RIGHTY;
}
} // namespace Ship
