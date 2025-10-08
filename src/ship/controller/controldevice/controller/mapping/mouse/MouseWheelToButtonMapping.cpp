#include "ship/controller/controldevice/controller/mapping/mouse/MouseWheelToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
MouseWheelToButtonMapping::MouseWheelToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                     WheelDirection wheelDirection)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseWheelToAnyMapping(wheelDirection),
      ControllerButtonMapping(PhysicalDeviceType::Mouse, portIndex, bitmask) {
}

void MouseWheelToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        return;
    }

    WheelDirections directions = WheelHandler::GetInstance()->GetDirections();
    if (mWheelDirection == directions.x || mWheelDirection == directions.y) {
        padButtons |= mBitmask;
    }
}

int8_t MouseWheelToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}

std::string MouseWheelToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-WHEEL%d", mPortIndex, mBitmask, mWheelDirection);
}

void MouseWheelToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    Ship::Context::GetInstance()->GetConsoleVariables()->SetString(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "MouseWheelToButtonMapping");
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(), static_cast<int>(mWheelDirection));
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void MouseWheelToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str());

    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

std::string MouseWheelToButtonMapping::GetPhysicalDeviceName() {
    return MouseWheelToAnyMapping::GetPhysicalDeviceName();
}

std::string MouseWheelToButtonMapping::GetPhysicalInputName() {
    return MouseWheelToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
