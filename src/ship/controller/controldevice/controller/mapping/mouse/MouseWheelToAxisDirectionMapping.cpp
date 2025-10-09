#include "ship/controller/controldevice/controller/mapping/mouse/MouseWheelToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/Context.h"
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"
#include "ship/controller/controldeck/ControlDeck.h"

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
    Ship::Context::GetInstance()->GetConsoleVariables()->SetString(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
        "MouseWheelToAxisDirectionMapping");
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(), static_cast<int>(mWheelDirection));
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void MouseWheelToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
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
