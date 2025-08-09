#include "controller/controldevice/controller/mapping/gcadapter/GCAdapterAxisDirectionToButtonMapping.h"
#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"
#include "Context.h"
#include "controller/controldeck/ControlDeck.h"
#include "controller/controldevice/controller/mapping/sdl/SDLMapping.h"

namespace Ship {
GCAdapterAxisDirectionToButtonMapping::GCAdapterAxisDirectionToButtonMapping(uint8_t portIndex,
                                                                             CONTROLLERBUTTONS_T bitmask,
                                                                             uint8_t gcAxis, int32_t axisDirection)
    : ControllerInputMapping(PhysicalDeviceType::GCAdapter),
      ControllerButtonMapping(PhysicalDeviceType::GCAdapter, portIndex, bitmask),
      GCAdapterAxisDirectionToAnyMapping(gcAxis, axisDirection) {
}

void GCAdapterAxisDirectionToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return;
    }

    GCPadStatus status = GCAdapter::Input(mPortIndex);

    uint8_t rawValue = 0;
    switch (mGcAxis) {
        case 0:
            rawValue = status.stickX;
            break; // Main Stick X
        case 1:
            rawValue = status.stickY;
            break; // Main Stick Y
        case 2:
            rawValue = status.substickX;
            break; // C-Stick X
        case 3:
            rawValue = status.substickY;
            break; // C-Stick Y
    }

    // 3. Convert the 0-255 value to a signed -128 to 127 range
    int16_t axisValue = static_cast<int16_t>(rawValue) - 128;

    // 4. Check if the value exceeds a threshold (e.g., 70)
    // This is a simpler replacement for the complex GetGlobal...Settings logic
    const int16_t axisThreshold = 60;

    if ((mAxisDirection == POSITIVE && axisValue > axisThreshold) ||
        (mAxisDirection == NEGATIVE && axisValue < -axisThreshold)) {
        padButtons |= mBitmask;
    }
}

int8_t GCAdapterAxisDirectionToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string GCAdapterAxisDirectionToButtonMapping::GetButtonMappingId() {
    // FIX: Use mGcAxis instead of the non-existent mControllerAxis
    return StringHelper::Sprintf("P%d-B%d-GCA%d-AD%s", mPortIndex, mBitmask, mGcAxis, mAxisDirection == 1 ? "P" : "N");
}

void GCAdapterAxisDirectionToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarSetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                  "GCAdapterAxisDirectionToButtonMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    CVarSetInteger(StringHelper::Sprintf("%s.GCAxis", mappingCvarKey.c_str()).c_str(), mGcAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void GCAdapterAxisDirectionToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    CVarClear(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.GCAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

std::string GCAdapterAxisDirectionToButtonMapping::GetPhysicalDeviceName() {
    return GCAdapterAxisDirectionToAnyMapping::GetPhysicalDeviceName();
}

std::string GCAdapterAxisDirectionToButtonMapping::GetPhysicalInputName() {
    return GCAdapterAxisDirectionToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship