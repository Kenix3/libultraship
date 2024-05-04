#include "SDLAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

#define MAX_SDL_RANGE (float)INT16_MAX

namespace Ship {
SDLAxisDirectionToAxisDirectionMapping::SDLAxisDirectionToAxisDirectionMapping(ShipDeviceIndex shipDeviceIndex,
                                                                               uint8_t portIndex, Stick stick,
                                                                               Direction direction,
                                                                               int32_t sdlControllerAxis,
                                                                               int32_t axisDirection)
    : ControllerInputMapping(shipDeviceIndex),
      ControllerAxisDirectionMapping(shipDeviceIndex, portIndex, stick, direction),
      SDLAxisDirectionToAnyMapping(shipDeviceIndex, sdlControllerAxis, axisDirection) {
}

float SDLAxisDirectionToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (!ControllerLoaded()) {
        return 0.0f;
    }

    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return 0.0f;
    }

    const auto axisValue = SDL_GameControllerGetAxis(mController, mControllerAxis);

    if ((mAxisDirection == POSITIVE && axisValue < 0) || (mAxisDirection == NEGATIVE && axisValue > 0)) {
        return 0.0f;
    }

    // scale {-32768 ... +32767} to {-MAX_AXIS_RANGE ... +MAX_AXIS_RANGE}
    // and return the absolute value of it
    return fabs(axisValue * MAX_AXIS_RANGE / MAX_SDL_RANGE);
}

std::string SDLAxisDirectionToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-LUSI%d-SDLA%d-AD%s", mPortIndex, mStick, mDirection,
                                 ControllerInputMapping::mShipDeviceIndex, mControllerAxis,
                                 mAxisDirection == 1 ? "P" : "N");
}

void SDLAxisDirectionToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLAxisDirectionToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mShipDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), mControllerAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void SDLAxisDirectionToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

uint8_t SDLAxisDirectionToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace Ship
