#include "KeyboardKeyToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace Ship {
KeyboardKeyToButtonMapping::KeyboardKeyToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                       KbScancode scancode)
    : ControllerInputMapping(ShipDeviceIndex::Keyboard), KeyboardKeyToAnyMapping(scancode),
      ControllerButtonMapping(ShipDeviceIndex::Keyboard, portIndex, bitmask) {
}

void KeyboardKeyToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->KeyboardGameInputBlocked()) {
        return;
    }

    if (!mKeyPressed) {
        return;
    }

    padButtons |= mBitmask;
}

uint8_t KeyboardKeyToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_KEYBOARD;
}

std::string KeyboardKeyToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-KB%d", mPortIndex, mBitmask, mKeyboardScancode);
}

void KeyboardKeyToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "KeyboardKeyToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), mKeyboardScancode);
    CVarSave();
}

void KeyboardKeyToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace Ship
