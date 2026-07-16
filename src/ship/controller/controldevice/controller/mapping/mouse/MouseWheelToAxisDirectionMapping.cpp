#include "ship/controller/controldevice/controller/mapping/mouse/MouseWheelToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
MouseWheelToAxisDirectionMapping::MouseWheelToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex,
                                                                   Direction direction, WheelDirection wheelDirection,
                                                                   std::shared_ptr<ControlDeck> controlDeck,
                                                                   std::shared_ptr<ConsoleVariable> consoleVariable)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseWheelToAnyMapping(wheelDirection),
      ControllerAxisDirectionMapping(PhysicalDeviceType::Mouse, portIndex, stickIndex, direction, controlDeck) {
    mConsoleVariable = std::move(consoleVariable);
    mControlDeck = std::move(controlDeck);
}

float MouseWheelToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (mControlDeck->MouseGameInputBlocked()) {
        return 0.0f;
    }

    if (mControlDeck->GetWheelHandler()->GetBufferedDirectionValue(mWheelDirection) > 0) {
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
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                                "MouseWheelToAxisDirectionMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(),
                                 static_cast<int>(mWheelDirection));
    mConsoleVariable->Save();
}

void MouseWheelToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->Save();
}

int8_t MouseWheelToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}

std::string MouseWheelToAxisDirectionMapping::GetPhysicalDeviceName() {
    return MouseWheelToAnyMapping::GetPhysicalDeviceName();
}

std::string MouseWheelToAxisDirectionMapping::GetPhysicalInputName() {
    return MouseWheelToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
