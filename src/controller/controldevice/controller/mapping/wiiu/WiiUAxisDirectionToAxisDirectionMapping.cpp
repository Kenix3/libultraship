#ifdef __WIIU__
#include "WiiUAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

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
    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return 0.0f;
    }

    float wiiUAxisValue = 0.0f;

    if (IsGamepad()) {
        VPADReadError error;
        VPADStatus* status = LUS::WiiU::GetVPADStatus(&error);
        if (status == nullptr) {
            return 0.0f;
        }

        switch (mControllerAxis) {
            case WII_U_AXIS_LEFT_STICK_X:
                wiiUAxisValue = status->leftStick.x;
                break;
            case WII_U_AXIS_LEFT_STICK_Y:
                wiiUAxisValue = status->leftStick.y * -1.0;
                break;
            case WII_U_AXIS_RIGHT_STICK_X:
                wiiUAxisValue = status->rightStick.x;
                break;
            case WII_U_AXIS_RIGHT_STICK_Y:
                wiiUAxisValue = status->rightStick.y * -1.0;
                break;
        }
    } else {
        KPADError error;
        KPADStatus* status = LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(GetWiiUDeviceChannel()), &error);
        if (status == nullptr || error != KPAD_ERROR_OK) {
            return 0.0f;
        }

        if (status->extensionType != ExtensionType()) {
            return 0.0f;
        }

        switch (ExtensionType()) {
            case WPAD_EXT_PRO_CONTROLLER:
                switch (mControllerAxis) {
                    case WII_U_AXIS_LEFT_STICK_X:
                        wiiUAxisValue = status->pro.leftStick.x;
                        break;
                    case WII_U_AXIS_LEFT_STICK_Y:
                        wiiUAxisValue = status->pro.leftStick.y * -1.0;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_X:
                        wiiUAxisValue = status->pro.rightStick.x;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_Y:
                        wiiUAxisValue = status->pro.rightStick.y * -1.0;
                        break;
                }
                break;
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
                switch (mControllerAxis) {
                    case WII_U_AXIS_NUNCHUK_STICK_X:
                        wiiUAxisValue = status->nunchuck.stick.x;
                        break;
                    case WII_U_AXIS_NUNCHUK_STICK_Y:
                        wiiUAxisValue = status->nunchuck.stick.y * -1.0;
                        break;
                }
                break;
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                switch (mControllerAxis) {
                    case WII_U_AXIS_LEFT_STICK_X:
                        wiiUAxisValue = status->classic.leftStick.x;
                        break;
                    case WII_U_AXIS_LEFT_STICK_Y:
                        wiiUAxisValue = status->classic.leftStick.y;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_X:
                        wiiUAxisValue = status->classic.rightStick.x;
                        break;
                    case WII_U_AXIS_RIGHT_STICK_Y:
                        wiiUAxisValue = status->classic.rightStick.y;
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
                  "WiiUAxisDirectionToAxisDirectionMapping");
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

uint8_t WiiUAxisDirectionToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string WiiUAxisDirectionToAxisDirectionMapping::GetPhysicalInputName() {
    switch (mControllerAxis) {
        case WII_U_AXIS_LEFT_STICK_X:
            return StringHelper::Sprintf("Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case WII_U_AXIS_LEFT_STICK_Y:
            return StringHelper::Sprintf("Left Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case WII_U_AXIS_RIGHT_STICK_X:
            return StringHelper::Sprintf("Right Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case WII_U_AXIS_RIGHT_STICK_Y:
            return StringHelper::Sprintf("Right Stick %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
        case WII_U_AXIS_NUNCHUK_STICK_X:
            return StringHelper::Sprintf("Nunchuk %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_LEFT : ICON_FA_ARROW_RIGHT);
        case WII_U_AXIS_NUNCHUK_STICK_Y:
            return StringHelper::Sprintf("Nunchuk %s",
                                         mAxisDirection == NEGATIVE ? ICON_FA_ARROW_UP : ICON_FA_ARROW_DOWN);
    }

    return StringHelper::Sprintf("Axis %d %s", mControllerAxis,
                                 mAxisDirection == NEGATIVE ? ICON_FA_MINUS : ICON_FA_PLUS);
}

std::string WiiUAxisDirectionToAxisDirectionMapping::GetPhysicalDeviceName() {
    return GetWiiUDeviceName();
}

bool WiiUAxisDirectionToAxisDirectionMapping::PhysicalDeviceIsConnected() {
    return WiiUDeviceIsConnected();
}
} // namespace LUS
#endif
