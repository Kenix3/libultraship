#include "MouseKeyToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

namespace Ship {
KeyMouseToAxisDirectionMapping::MouseKeyToAxisDirectionMapping(uint8_t portIndex, Stick stick,
                                                                     Direction direction, MouseBtn button)
    : ControllerInputMapping(ShipDeviceIndex::Mouse), MouseKeyToAnyMapping(button),
      ControllerAxisDirectionMapping(ShipDeviceIndex::Mouse, portIndex, stick, direction) {
}

float MouseKeyToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        return 0.0f;
    }

    return mKeyPressed ? MAX_AXIS_RANGE : 0.0f;
}

std::string MouseKeyToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-MOUSE%d", mPortIndex, mStick, mDirection, mButton);
}

void MouseKeyToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "MouseKeyToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStick);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), static_cast<int>(mButton));
    CVarSave();
}

void MouseKeyToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

uint8_t MouseKeyToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}
} // namespace Ship
