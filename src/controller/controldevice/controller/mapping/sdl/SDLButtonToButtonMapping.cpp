#include "SDLButtonToButtonMapping.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace ShipDK {
SDLButtonToButtonMapping::SDLButtonToButtonMapping(ShipDKDeviceIndex shipDKDeviceIndex, uint8_t portIndex,
                                                   CONTROLLERBUTTONS_T bitmask, int32_t sdlControllerButton)
    : ControllerInputMapping(shipDKDeviceIndex), ControllerButtonMapping(shipDKDeviceIndex, portIndex, bitmask),
      SDLButtonToAnyMapping(shipDKDeviceIndex, sdlControllerButton) {
}

void SDLButtonToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (!ControllerLoaded()) {
        return;
    }

    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return;
    }

    if (SDL_GameControllerGetButton(mController, mControllerButton)) {
        padButtons |= mBitmask;
    }
}

uint8_t SDLButtonToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string SDLButtonToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-LUSI%d-SDLB%d", mPortIndex, mBitmask,
                                 ControllerInputMapping::mShipDKDeviceIndex, mControllerButton);
}

void SDLButtonToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "SDLButtonToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mShipDKDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), mControllerButton);
    CVarSave();
}

void SDLButtonToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + GetButtonMappingId();

    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str());
    CVarSave();
}
} // namespace ShipDK
