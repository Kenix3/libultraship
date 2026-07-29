#include "ship/controller/controldevice/controller/mapping/mouse/MouseWheelToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
MouseWheelToButtonMapping::MouseWheelToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                     WheelDirection wheelDirection,
                                                     std::shared_ptr<ControlDeck> controlDeck,
                                                     std::shared_ptr<ConsoleVariable> consoleVariable)
    : ControllerInputMapping(PhysicalDeviceType::Mouse), MouseWheelToAnyMapping(wheelDirection),
      ControllerButtonMapping(PhysicalDeviceType::Mouse, portIndex, bitmask, controlDeck) {
    mConsoleVariable = std::move(consoleVariable);
    mControlDeck = std::move(controlDeck);
}

void MouseWheelToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (mControlDeck->MouseGameInputBlocked()) {
        return;
    }

    auto directions = mControlDeck->GetWheelHandler()->GetDirections();
    if (mWheelDirection == directions.X || mWheelDirection == directions.Y) {
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
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                                "MouseWheelToButtonMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(),
                                 static_cast<int>(mWheelDirection));
    mConsoleVariable->Save();
}

void MouseWheelToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str());

    mConsoleVariable->Save();
}

std::string MouseWheelToButtonMapping::GetPhysicalDeviceName() {
    return MouseWheelToAnyMapping::GetPhysicalDeviceName();
}

std::string MouseWheelToButtonMapping::GetPhysicalInputName() {
    return MouseWheelToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
