#include "ship/controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
SDLButtonToButtonMapping::SDLButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                   int32_t sdlControllerButton,
                                                   std::shared_ptr<ControlDeck> controlDeck,
                                                   std::shared_ptr<ConsoleVariable> consoleVariable)
    : ControllerInputMapping(PhysicalDeviceType::SDLGamepad), SDLButtonToAnyMapping(sdlControllerButton),
      ControllerButtonMapping(PhysicalDeviceType::SDLGamepad, portIndex, bitmask, controlDeck) {
    mConsoleVariable = std::move(consoleVariable);
    mControlDeck = std::move(controlDeck);
}

void SDLButtonToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (mControlDeck->GamepadGameInputBlocked()) {
        return;
    }

    for (const auto& [instanceId, gamepad] :
         mControlDeck->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(mPortIndex)) {
        if (SDL_GameControllerGetButton(gamepad, mControllerButton)) {
            padButtons |= mBitmask;
            return;
        }
    }
}

int8_t SDLButtonToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string SDLButtonToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-SDLB%d", mPortIndex, mBitmask, mControllerButton);
}

void SDLButtonToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(),
                                "SDLButtonToButtonMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(),
                                 mControllerButton);
    mConsoleVariable->Save();
}

void SDLButtonToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->Save();
}

std::string SDLButtonToButtonMapping::GetPhysicalDeviceName() {
    return SDLButtonToAnyMapping::GetPhysicalDeviceName();
}

std::string SDLButtonToButtonMapping::GetPhysicalInputName() {
    return SDLButtonToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
