#ifdef __WIIU__
#include "WiiUButtonToButtonMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace Ship {
WiiUButtonToButtonMapping::WiiUButtonToButtonMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex,
                                                     CONTROLLERBUTTONS_T bitmask, bool isNunchuk, bool isClassic,
                                                     uint32_t wiiuControllerButton)
    : ControllerInputMapping(shipDeviceIndex), ControllerButtonMapping(shipDeviceIndex, portIndex, bitmask),
      WiiUButtonToAnyMapping(shipDeviceIndex, isNunchuk, isClassic, wiiuControllerButton) {
}

void WiiUButtonToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return;
    }

    if (PhysicalButtonIsPressed()) {
        padButtons |= mBitmask;
    }
}

uint8_t WiiUButtonToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string WiiUButtonToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-LUSI%d-N%d-C%d-B%d", mPortIndex, mBitmask,
                                 ControllerInputMapping::mShipDeviceIndex, mIsNunchukButton ? 1 : 0,
                                 mIsClassicControllerButton ? 1 : 0, mControllerButton);
}

void WiiUButtonToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "WiiUButtonToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mShipDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.IsClassicControllerButton", mappingCvarKey.c_str()).c_str(),
                   mIsNunchukButton ? 1 : 0);
    CVarSetInteger(StringHelper::Sprintf("%s.IsNunchukButton", mappingCvarKey.c_str()).c_str(),
                   mIsClassicControllerButton ? 1 : 0);
    CVarSetInteger(StringHelper::Sprintf("%s.WiiUControllerButton", mappingCvarKey.c_str()).c_str(), mControllerButton);
    CVarSave();
}

void WiiUButtonToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();

    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.IsClassicControllerButton", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.IsNunchukButton", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WiiUControllerButton", mappingCvarKey.c_str()).c_str());
    CVarSave();
}
} // namespace Ship
#endif
