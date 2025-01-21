#include "MouseWheelToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"
#include "WheelHandler.h"
#include "Context.h"

namespace Ship {
MouseWheelToButtonMapping::MouseWheelToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                     WheelDirection wheelDirection)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseWheelToAnyMapping(wheelDirection),
      ControllerButtonMapping(PhysicalDeviceType::Mouse, portIndex, bitmask) {
}

void MouseWheelToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        return;
    }

    WheelDirections directions = WheelHandler::GetInstance()->GetDirections();
    if (mWheelDirection == directions.x || mWheelDirection == directions.y) {
        padButtons |= mBitmask;
    }
}

int8_t MouseWheelToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}

std::string MouseWheelToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-WHEEL%d", mPortIndex, mBitmask, mWheelDirection);
}

void MouseWheelToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "MouseWheelToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(),
                   static_cast<int>(mWheelDirection));
    CVarSave();
}

void MouseWheelToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace Ship
