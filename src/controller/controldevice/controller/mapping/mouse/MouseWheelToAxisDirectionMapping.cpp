#include "MouseWheelToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include "utils/StringHelper.h"
#include "window/gui/IconsFontAwesome4.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"
#include "WheelHandler.h"

namespace Ship {
MouseWheelToAxisDirectionMapping::MouseWheelToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex,
                                                                   Direction direction, WheelDirection wheelDirection)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseWheelToAnyMapping(wheelDirection),
      ControllerAxisDirectionMapping(PhysicalDeviceType::Mouse, portIndex, stickIndex, direction) {
}

float MouseWheelToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        return 0.0f;
    }

    if (WheelHandler::GetInstance()->GetBufferedDirectionValue(mWheelDirection) > 0) {
        return MAX_AXIS_RANGE;
    } else {
        return 0.0f;
    }
}

std::string MouseWheelToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-WHEEL%d", mPortIndex, mStickIndex, mDirection, mWheelDirection);
}

void MouseWheelToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarSetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                  "MouseWheelToAxisDirectionMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    CVarSetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(),
                   static_cast<int>(mWheelDirection));
    CVarSave();
}

void MouseWheelToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    CVarClear(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str());
    CVarSave();
}

int8_t MouseWheelToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}
} // namespace Ship
