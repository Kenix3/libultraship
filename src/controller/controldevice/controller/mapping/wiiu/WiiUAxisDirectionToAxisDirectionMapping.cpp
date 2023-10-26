#ifdef __WIIU__
#include "WiiUAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

#define WII_U_AXIS_LEFT_STICK_X = 0
#define WII_U_AXIS_LEFT_STICK_Y = 1
#define WII_U_AXIS_RIGHT_STICK_X = 2
#define WII_U_AXIS_RIGHT_STICK_Y = 3
#define WII_U_AXIS_NUNCHUK_STICK_X = 4
#define WII_U_AXIS_NUNCHUK_STICK_Y = 5

namespace LUS {
WiiUAxisDirectionToAxisDirectionMapping::WiiUAxisDirectionToAxisDirectionMapping(LUSDeviceIndex lusDeviceIndex,
                                                                                 uint8_t portIndex, Stick stick,
                                                                                 Direction direction,
                                                                                 int32_t wiiuControllerAxis,
                                                                                 int32_t axisDirection)
    : ControllerInputMapping(lusDeviceIndex),
      ControllerAxisDirectionMapping(lusDeviceIndex, portIndex, stick, direction), WiiUMapping(lusDeviceIndex),
      mControllerAxis(wiiuControllerAxis) {
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}

float WiiUAxisDirectionToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (!ControllerLoaded()) {
        return 0.0f;
    }

    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return 0.0f;
    }

    float wiiUAxisValue = 0.0f;

    if (IsGamepad()) {
        switch (mControllerAxis) {
            case WII_U_AXIS_LEFT_STICK_X:
                wiiUAxisValue = mWiiUGamepadController->leftStick.x;
                break;
            case WII_U_AXIS_LEFT_STICK_Y:
                wiiUAxisValue = mWiiUGamepadController->leftStick.y;
                break;
            case WII_U_AXIS_RIGHT_STICK_X:
                wiiUAxisValue = mWiiUGamepadController->rightStick.x;
                break;
            case WII_U_AXIS_RIGHT_STICK_Y:
                wiiUAxisValue = mWiiUGamepadController->rightStick.y;
                break;
        }
    } else {
        switch (ExtensionType()) {
            case WPAD_EXT_PRO_CONTROLLER:
                switch (mControllerAxis) {
                    case WII_U_AXIS_LEFT_STICK_X:
                        wiiUAxisValue = mController->pro.leftStick.x;
                        break;
                    case WII_U_AXIS_LEFT_STICK_Y:
                        wiiUAxisValue = mController->pro.leftStick.y;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_X:
                        wiiUAxisValue = mController->pro.rightStick.x;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_Y:
                        wiiUAxisValue = mController->pro.rightStick.y;
                        break;
                }
                break;
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
                switch (mControllerAxis) {
                    case WII_U_AXIS_LEFT_STICK_X:
                        wiiUAxisValue = mController->nunchuck.leftStick.x;
                        break;
                    case WII_U_AXIS_LEFT_STICK_Y:
                        wiiUAxisValue = mController->nunchuck.leftStick.y;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_X:
                        wiiUAxisValue = mController->nunchuck.rightStick.x;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_Y:
                        wiiUAxisValue = mController->nunchuck.rightStick.y;
                        break;
                }
                break;
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                switch (mControllerAxis) {
                    case WII_U_AXIS_LEFT_STICK_X:
                        wiiUAxisValue = mController->classic.leftStick.x;
                        break;
                    case WII_U_AXIS_LEFT_STICK_Y:
                        wiiUAxisValue = mController->classic.leftStick.y;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_X:
                        wiiUAxisValue = mController->classic.rightStick.x;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_Y:
                        wiiUAxisValue = mController->classic.rightStick.y;
                        break;
                }
                break;
        }
    }

    if ((mAxisDirection == POSITIVE && wiiUAxisValue < 0) || (mAxisDirection == NEGATIVE && wiiUAxisValue > 0)) {
        return 0.0f;
    }

    // scale {-1.0 ... +1.0} to {-MAX_AXIS_RANGE ... +MAX_AXIS_RANGE}
    // and return the absolute value of it
    return fabs(wiiUAxisValue * MAX_AXIS_RANGE);
}

std::string WiiUAxisDirectionToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-LUSI%d-A%d-AD%s", mPortIndex, mStick, mDirection,
                                 ControllerInputMapping::mLUSDeviceIndex, mControllerAxis,
                                 mAxisDirection == 1 ? "P" : "N");
}

void WiiUAxisDirectionToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLAxisDirectionToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mLUSDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.WiiUControllerAxis", mappingCvarKey.c_str()).c_str(), mControllerAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void WiiUAxisDirectionToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WiiUControllerAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

uint8_t SDLAxisDirectionToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace LUS
#endif
