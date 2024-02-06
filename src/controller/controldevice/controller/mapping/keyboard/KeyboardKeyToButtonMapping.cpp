#include "KeyboardKeyToButtonMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace LUS {
KeyboardKeyToButtonMapping::KeyboardKeyToButtonMapping(uint8_t portIndex, uint16_t bitmask, KbScancode scancode)
    : ControllerInputMapping(LUSDeviceIndex::Keyboard),
      ControllerButtonMapping(LUSDeviceIndex::Keyboard, portIndex, bitmask), KeyboardKeyToAnyMapping(scancode) {
}

void KeyboardKeyToButtonMapping::UpdatePad(uint16_t& padButtons) {
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
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "KeyboardKeyToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), mKeyboardScancode);
    CVarSave();
}

void KeyboardKeyToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();

    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace LUS
