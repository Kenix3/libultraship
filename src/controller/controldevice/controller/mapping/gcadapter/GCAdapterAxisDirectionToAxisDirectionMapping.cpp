#include "controller/controldevice/controller/mapping/gcadapter/GCAdapterAxisDirectionToAxisDirectionMapping.h"
#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"
#include "Context.h"
#include "controller/controldeck/ControlDeck.h"
#include "controller/controldevice/controller/mapping/sdl/SDLMapping.h"

namespace Ship {
GCAdapterAxisDirectionToAxisDirectionMapping::GCAdapterAxisDirectionToAxisDirectionMapping(
    uint8_t portIndex, StickIndex stickIndex, Direction direction, uint8_t gcAxis, int32_t axisDirection)
    : ControllerInputMapping(PhysicalDeviceType::GCAdapter),
      ControllerAxisDirectionMapping(PhysicalDeviceType::GCAdapter, portIndex, stickIndex, direction),
      GCAdapterAxisDirectionToAnyMapping(gcAxis, axisDirection) {
}

float GCAdapterAxisDirectionToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return 0.0f;
    }

    // Get the current state of the GC controller for our port
    GCPadStatus status = GCAdapter::Input(mPortIndex);

    // Select the correct raw axis value from the GCPadStatus struct
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

    float normalizedValue = static_cast<float>(rawValue - 128) / 100.0f;
    normalizedValue *= 85;

    if ((mAxisDirection == POSITIVE && normalizedValue < 0) || (mAxisDirection == NEGATIVE && normalizedValue > 0)) {
        return 0.0f;
    }

    return std::abs(normalizedValue);
}

void GCAdapterAxisDirectionToAxisDirectionMapping::RemapStick(double& x, double& y) {
    // Remaps the control stick into more of a squircle, à la electromodder's adapter

    if (x == 0.0 && y == 0.0) {
        return;
    }
    double max_boost = 0.28;

    double abs_x = abs(x);
    double abs_y = abs(y);

    double max_abs = std::max(abs_x, abs_y);

    if (max_abs == 0) {
        return;
    }
    double diag_ratio = std::min(abs_x, abs_y) / max_abs;

    double boost_strength = sin(diag_ratio * 1.57);
    double magnitude = sqrt(pow(x, 2) + pow(y, 2));

    double boost_factor = 1.0 + (max_boost * boost_strength * (magnitude / 100.0));

    x = x * boost_factor;
    y = y * boost_factor;
}

std::string GCAdapterAxisDirectionToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-GCA%d-AD%s", mPortIndex, mStickIndex, mDirection, mGcAxis,
                                 mAxisDirection == 1 ? "P" : "N");
}

void GCAdapterAxisDirectionToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "GCAdapterAxisDirectionToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.GCAxis", mappingCvarKey.c_str()).c_str(), mGcAxis);
    CVarSetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), mAxisDirection);
    CVarSave();
}

void GCAdapterAxisDirectionToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.GCAxis", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

int8_t GCAdapterAxisDirectionToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}
} // namespace Ship