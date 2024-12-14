#include "MouseKeyToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace Ship {
MouseKeyToButtonMapping::MouseKeyToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                       MouseBtn button)
    : ControllerInputMapping(ShipDeviceIndex::Mouse), MouseKeyToAnyMapping(button),
      ControllerButtonMapping(ShipDeviceIndex::Mouse, portIndex, bitmask) {
}

void MouseKeyToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        return;
    }

    if (!mKeyPressed) {
        return;
    }

    padButtons |= mBitmask;
}

uint8_t MouseKeyToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}

std::string MouseKeyToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-MOUSE%d", mPortIndex, mBitmask, mButton);
}

void MouseKeyToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "MouseKeyToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), static_cast<int>(mButton));
    CVarSave();
}

void MouseKeyToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace Ship
