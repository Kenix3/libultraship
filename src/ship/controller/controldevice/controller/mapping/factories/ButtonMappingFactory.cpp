#include "ship/controller/controldevice/controller/mapping/factories/ButtonMappingFactory.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/mouse/MouseButtonToButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/mouse/MouseWheelToButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"
#include "ship/controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
std::shared_ptr<ControllerButtonMapping> ButtonMappingFactory::CreateButtonMappingFromConfig(
    uint8_t portIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
    std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window) {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + id;
    const std::string mappingClass =
        consoleVariable->GetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "");
    CONTROLLERBUTTONS_T bitmask =
        consoleVariable->GetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), 0);
    if (!bitmask) {
        consoleVariable->ClearVariable(mappingCvarKey.c_str());
        consoleVariable->Save();
        return nullptr;
    }

    if (mappingClass == "SDLButtonToButtonMapping") {
        int32_t sdlControllerButton = consoleVariable->GetInteger(
            StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if (sdlControllerButton == -1) {
            consoleVariable->ClearVariable(mappingCvarKey.c_str());
            consoleVariable->Save();
            return nullptr;
        }

        return std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, sdlControllerButton, controlDeck,
                                                          consoleVariable);
    }

    if (mappingClass == "SDLAxisDirectionToButtonMapping") {
        int32_t sdlControllerAxis = consoleVariable->GetInteger(
            StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), -1);
        int32_t axisDirection =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerAxis == -1 || (axisDirection != -1 && axisDirection != 1)) {
            consoleVariable->ClearVariable(mappingCvarKey.c_str());
            consoleVariable->Save();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, bitmask, sdlControllerAxis, axisDirection,
                                                                 controlDeck, consoleVariable);
    }

    if (mappingClass == "KeyboardKeyToButtonMapping") {
        int32_t scancode = consoleVariable->GetInteger(
            StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<KeyboardKeyToButtonMapping>(portIndex, bitmask, static_cast<KbScancode>(scancode),
                                                            controlDeck, window, consoleVariable);
    }

    if (mappingClass == "MouseButtonToButtonMapping") {
        int mouseButton =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<MouseButtonToButtonMapping>(portIndex, bitmask, static_cast<MouseBtn>(mouseButton),
                                                            controlDeck, consoleVariable);
    }

    if (mappingClass == "MouseWheelToButtonMapping") {
        int wheelDirection =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<MouseWheelToButtonMapping>(
            portIndex, bitmask, static_cast<WheelDirection>(wheelDirection), controlDeck, consoleVariable);
    }

    return nullptr;
}

std::vector<std::shared_ptr<ControllerButtonMapping>> ButtonMappingFactory::CreateDefaultKeyboardButtonMappings(
    uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, std::shared_ptr<ConsoleVariable> consoleVariable,
    std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    auto defaultsForBitmask =
        controlDeck->GetControllerDefaultMappings()->GetDefaultKeyboardKeyToButtonMappings()[bitmask];

    for (const auto& scancode : defaultsForBitmask) {
        mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, bitmask, scancode, controlDeck,
                                                                        window, consoleVariable));
    }

    return mappings;
}

std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultSDLButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                     std::shared_ptr<ConsoleVariable> consoleVariable,
                                                     std::shared_ptr<ControlDeck> controlDeck) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    auto defaultButtonsForBitmask =
        controlDeck->GetControllerDefaultMappings()->GetDefaultSDLButtonToButtonMappings()[bitmask];

    for (const auto& sdlGamepadButton : defaultButtonsForBitmask) {
        mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, sdlGamepadButton, controlDeck,
                                                                      consoleVariable));
    }

    auto defaultAxisDirectionsForBitmask =
        controlDeck->GetControllerDefaultMappings()->GetDefaultSDLAxisDirectionToButtonMappings()[bitmask];

    for (const auto& [sdlGamepadAxis, axisDirection] : defaultAxisDirectionsForBitmask) {
        mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(
            portIndex, bitmask, sdlGamepadAxis, axisDirection, controlDeck, consoleVariable));
    }

    return mappings;
}

std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromSDLInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                      std::shared_ptr<ConsoleVariable> consoleVariable,
                                                      std::shared_ptr<ControlDeck> controlDeck) {
    std::shared_ptr<ControllerButtonMapping> mapping = nullptr;

    for (auto [instanceId, gamepad] :
         controlDeck->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(portIndex)) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, button, controlDeck,
                                                                     consoleVariable);
                break;
            }
        }

        if (mapping != nullptr) {
            break;
        }

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            const auto axis = static_cast<SDL_GameControllerAxis>(i);
            const auto axisValue = SDL_GameControllerGetAxis(gamepad, axis) / 32767.0f;
            int32_t axisDirection = 0;
            if (axisValue < -0.7f) {
                axisDirection = NEGATIVE;
            } else if (axisValue > 0.7f) {
                axisDirection = POSITIVE;
            }

            if (axisDirection == 0) {
                continue;
            }

            mapping = std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, bitmask, axis, axisDirection,
                                                                        controlDeck, consoleVariable);
            break;
        }
    }

    return mapping;
}

std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromMouseWheelInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask,
                                                             std::shared_ptr<ConsoleVariable> consoleVariable,
                                                             std::shared_ptr<ControlDeck> controlDeck) {
    auto wheelDirections = controlDeck->GetWheelHandler()->GetDirections();
    WheelDirection wheelDirection;
    if (wheelDirections.X != LUS_WHEEL_NONE) {
        wheelDirection = wheelDirections.X;
    } else if (wheelDirections.Y != LUS_WHEEL_NONE) {
        wheelDirection = wheelDirections.Y;
    } else {
        return nullptr;
    }

    return std::make_shared<MouseWheelToButtonMapping>(portIndex, bitmask, wheelDirection, controlDeck,
                                                       consoleVariable);
}
} // namespace Ship
