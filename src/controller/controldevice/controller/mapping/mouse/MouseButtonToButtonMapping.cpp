#include "MouseButtonToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace Ship {
MouseButtonToButtonMapping::MouseButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, MouseBtn button)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseButtonToAnyMapping(button),
      ControllerButtonMapping(PhysicalDeviceType::Mouse, portIndex, bitmask) {
}

void MouseButtonToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        return;
    }

    if (!mKeyPressed) {
        return;
    }

    padButtons |= mBitmask;
}

int8_t MouseButtonToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}

std::string MouseButtonToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-MOUSE%d", mPortIndex, mBitmask, mButton);
}

void MouseButtonToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "MouseButtonToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), static_cast<int>(mButton));
    CVarSave();
}

void MouseButtonToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace Ship
