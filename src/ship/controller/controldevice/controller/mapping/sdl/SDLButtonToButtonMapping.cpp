#include "ship/controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/Context.h"

namespace Ship {
SDLButtonToButtonMapping::SDLButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                   int32_t sdlControllerButton)
    : ControllerInputMapping(PhysicalDeviceType::SDLGamepad), SDLButtonToAnyMapping(sdlControllerButton),
      ControllerButtonMapping(PhysicalDeviceType::SDLGamepad, portIndex, bitmask) {
}

void SDLButtonToButtonMapping::UpdatePad(CONTROLLERBUTTONS_T& padButtons) {
    if (Context::GetRawInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        return;
    }

    for (const auto& [instanceId, gamepad] : Context::GetRawInstance()
                                                 ->GetControlDeck()
                                                 ->GetConnectedPhysicalDeviceManager()
                                                 ->GetConnectedSDLGamepadsForPort(mPortIndex)) {
        if (SDL_GameControllerGetButton(gamepad, mControllerButton)) {
            padButtons |= mBitmask;
            return;
        }
    }
}

int8_t SDLButtonToButtonMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

ImGuiKey SDLButtonToButtonMapping::GetGamepadKey() {
    switch (mControllerButton) {
        case SDL_CONTROLLER_BUTTON_A:
            return ImGuiKey_GamepadFaceDown;
        case SDL_CONTROLLER_BUTTON_B:
            return ImGuiKey_GamepadFaceRight;
        case SDL_CONTROLLER_BUTTON_X:
            return ImGuiKey_GamepadFaceLeft;
        case SDL_CONTROLLER_BUTTON_Y:
            return ImGuiKey_GamepadFaceUp;
        case SDL_CONTROLLER_BUTTON_BACK:
            return ImGuiKey_GamepadBack;
        case SDL_CONTROLLER_BUTTON_START:
            return ImGuiKey_GamepadStart;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            return ImGuiKey_GamepadL3;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            return ImGuiKey_GamepadR3;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return ImGuiKey_GamepadL1;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return ImGuiKey_GamepadR1;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return ImGuiKey_GamepadDpadUp;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return ImGuiKey_GamepadDpadDown;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return ImGuiKey_GamepadDpadLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return ImGuiKey_GamepadDpadRight;
        default:
            return ImGuiKey_None;
    }
}

std::string SDLButtonToButtonMapping::GetButtonMappingId() {
    return StringHelper::Sprintf("P%d-B%d-SDLB%d", mPortIndex, mBitmask, mControllerButton);
}

void SDLButtonToButtonMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();
    Ship::Context::GetRawInstance()->GetConsoleVariables()->SetString(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "SDLButtonToButtonMapping");
    Ship::Context::GetRawInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), mBitmask);
    Ship::Context::GetRawInstance()->GetConsoleVariables()->SetInteger(
        StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), mControllerButton);
    Ship::Context::GetRawInstance()->GetConsoleVariables()->Save();
}

void SDLButtonToButtonMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + GetButtonMappingId();

    Ship::Context::GetRawInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetRawInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetRawInstance()->GetConsoleVariables()->ClearVariable(
        StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str());
    Ship::Context::GetRawInstance()->GetConsoleVariables()->Save();
}

std::string SDLButtonToButtonMapping::GetPhysicalDeviceName() {
    return SDLButtonToAnyMapping::GetPhysicalDeviceName();
}

std::string SDLButtonToButtonMapping::GetPhysicalInputName() {
    return SDLButtonToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
