#include "controller/controldevice/controller/mapping/gcadapter/GCAdapterButtonToButtonMapping.h"
#include <utils/StringHelper.h>
#include <public/bridge/consolevariablebridge.h>

namespace Ship {
GCAdapterButtonToButtonMapping::GCAdapterButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                               CONTROLLERBUTTONS_T gcButton)
    : ControllerInputMapping(PhysicalDeviceType::GCAdapter), GCAdapterButtonToAnyMapping(gcButton),
      ControllerButtonMapping(PhysicalDeviceType::GCAdapter, portIndex, bitmask), mGcButton(gcButton) {
}

void GCAdapterButtonToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    // Get current state of GC controller
    GCPadStatus status = GCAdapter::Input(mPortIndex);

    if (status.button & mGcButton) {
        padButtons |= mBitmask;
    }
}

std::string GCAdapterButtonToButtonMapping::GetButtonMappingId() {
    // Create a unique ID for this mapping based on port, N64 button, and GC button
    return StringHelper::Sprintf("P%d-B%d-GCB%d", mPortIndex, mBitmask, mGcButton);
}

void GCAdapterButtonToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "GCAdapterButtonToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.GCButton", mappingCvarKey.c_str()).c_str(), mGcButton);
    CVarSave();
}

void GCAdapterButtonToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.GCButton", mappingCvarKey.c_str()).c_str());
    CVarSave();
}
} // namespace Ship