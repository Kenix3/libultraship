#include "ship/controller/controldevice/controller/mapping/mouse/MouseButtonToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/config/Config.h"

namespace Ship {
MouseButtonToAxisDirectionMapping::MouseButtonToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex,
                                                                     Direction direction, MouseBtn button,
                                                                     std::shared_ptr<ControlDeck> controlDeck,
                                                                     std::shared_ptr<Config> config,
                                                                     std::shared_ptr<ConsoleVariable> consoleVariable)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseButtonToAnyMapping(button),
      ControllerAxisDirectionMapping(PhysicalDeviceType::Mouse, portIndex, stickIndex, direction, controlDeck, config) {
    mConsoleVariable = std::move(consoleVariable);
    mControlDeck = std::move(controlDeck);
}

float MouseButtonToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (mControlDeck->MouseGameInputBlocked()) {
        return 0.0f;
    }

    return mKeyPressed ? MAX_AXIS_RANGE : 0.0f;
}

std::string MouseButtonToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-MOUSE%d", mPortIndex, mStickIndex, mDirection, mButton);
}

void MouseButtonToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                                "MouseButtonToAxisDirectionMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(),
                                 static_cast<int>(mButton));
    mConsoleVariable->Save();
}

void MouseButtonToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->Save();
}

int8_t MouseButtonToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}

std::string MouseButtonToAxisDirectionMapping::GetPhysicalDeviceName() {
    return MouseButtonToAnyMapping::GetPhysicalDeviceName();
}

std::string MouseButtonToAxisDirectionMapping::GetPhysicalInputName() {
    return MouseButtonToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
