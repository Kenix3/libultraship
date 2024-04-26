#include "SDLButtonToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

#define MAX_SDL_RANGE (float)INT16_MAX

namespace ShipDK {
SDLButtonToAxisDirectionMapping::SDLButtonToAxisDirectionMapping(ShipDKDeviceIndex shipDKDeviceIndex, uint8_t portIndex,
                                                                 Stick stick, Direction direction,
                                                                 int32_t sdlControllerButton)
    : ControllerInputMapping(shipDKDeviceIndex),
      ControllerAxisDirectionMapping(shipDKDeviceIndex, portIndex, stick, direction),
      SDLButtonToAnyMapping(shipDKDeviceIndex, sdlControllerButton) {
}

float SDLButtonToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (ControllerLoaded() && !Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked() &&
        SDL_GameControllerGetButton(mController, mControllerButton)) {
        return MAX_AXIS_RANGE;
    }

    return 0.0f;
}

std::string SDLButtonToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-LUSI%d-SDLB%d", mPortIndex, mStick, mDirection,
                                 ControllerInputMapping::mShipDKDeviceIndex, mControllerButton);
}

void SDLButtonToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLButtonToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mShipDKDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), mControllerButton);
    CVarSave();
}

void SDLButtonToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

uint8_t SDLButtonToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace ShipDK
