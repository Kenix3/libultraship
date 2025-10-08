#include "ship/controller/controldevice/controller/mapping/mouse/MouseButtonToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/Context.h"

namespace Ship {
MouseButtonToButtonMapping::MouseButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, MouseBtn button)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseButtonToAnyMapping(button),
      ControllerButtonMapping(PhysicalDeviceType::Mouse, portIndex, bitmask) {
}

void MouseButtonToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetInstance()->GetControlDeck()->MouseGameInputBlocked()) {
        return;
    }

    if (!mKeyPressed) {
        return;
    }

    padButtons |= mBitmask;
}

int8_t MouseButtonToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_MOUSE;
}

std::string MouseButtonToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-MOUSE%d", mPortIndex, mBitmask, mButton);
}

void MouseButtonToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    Ship::Context::GetInstance()->GetConsoleVariables()->SetString(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "MouseButtonToButtonMapping");
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), static_cast<int>(mButton));
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

void MouseButtonToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str());

    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
}

std::string MouseButtonToButtonMapping::GetPhysicalDeviceName() {
    return MouseButtonToAnyMapping::GetPhysicalDeviceName();
}

std::string MouseButtonToButtonMapping::GetPhysicalInputName() {
    return MouseButtonToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
