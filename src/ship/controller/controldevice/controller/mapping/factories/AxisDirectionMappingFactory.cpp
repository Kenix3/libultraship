#include "ship/controller/controldevice/controller/mapping/factories/AxisDirectionMappingFactory.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h"
#include "ship/controller/controldevice/controller/mapping/mouse/MouseButtonToAxisDirectionMapping.h"
#include "ship/controller/controldevice/controller/mapping/mouse/MouseWheelToAxisDirectionMapping.h"
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"

#include "ship/controller/controldevice/controller/mapping/sdl/SDLButtonToAxisDirectionMapping.h"
#include "ship/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.h"

#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"

#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
std::shared_ptr<ControllerAxisDirectionMapping> AxisDirectionMappingFactory::CreateAxisDirectionMappingFromConfig(
    uint8_t portIndex, StickIndex stickIndex, std::string id, std::shared_ptr<ConsoleVariable> consoleVariable,
    std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window) {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + id;
    const std::string mappingClass = consoleVariable->GetString(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "SDLAxisDirectionToAxisDirectionMapping") {
        int32_t direction =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerAxis = consoleVariable->GetInteger(
            StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), -1);
        int32_t axisDirection =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) ||
            sdlControllerAxis == -1 || (axisDirection != NEGATIVE && axisDirection != POSITIVE)) {
            consoleVariable->ClearVariable(mappingCvarKey.c_str());
            consoleVariable->Save();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), sdlControllerAxis, axisDirection, controlDeck,
            consoleVariable);
    }

    if (mappingClass == "SDLButtonToAxisDirectionMapping") {
        int32_t direction =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerButton = consoleVariable->GetInteger(
            StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) ||
            sdlControllerButton == -1) {
            consoleVariable->ClearVariable(mappingCvarKey.c_str());
            consoleVariable->Save();
            return nullptr;
        }

        return std::make_shared<SDLButtonToAxisDirectionMapping>(portIndex, stickIndex,
                                                                 static_cast<Direction>(direction), sdlControllerButton,
                                                                 controlDeck, consoleVariable);
    }

    if (mappingClass == "KeyboardKeyToAxisDirectionMapping") {
        int32_t direction =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t scancode = consoleVariable->GetInteger(
            StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), 0);

        if (direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) {
            consoleVariable->ClearVariable(mappingCvarKey.c_str());
            consoleVariable->Save();
            return nullptr;
        }

        return std::make_shared<KeyboardKeyToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), static_cast<KbScancode>(scancode), controlDeck,
            window, consoleVariable);
    }

    if (mappingClass == "MouseButtonToAxisDirectionMapping") {
        int32_t direction =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int mouseButton =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), 0);

        if (direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) {
            consoleVariable->ClearVariable(mappingCvarKey.c_str());
            consoleVariable->Save();
            return nullptr;
        }

        return std::make_shared<MouseButtonToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), static_cast<MouseBtn>(mouseButton), controlDeck,
            consoleVariable);
    }

    if (mappingClass == "MouseWheelToAxisDirectionMapping") {
        int32_t direction =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int wheelDirection =
            consoleVariable->GetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) {
            consoleVariable->ClearVariable(mappingCvarKey.c_str());
            consoleVariable->Save();
            return nullptr;
        }

        return std::make_shared<MouseWheelToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), static_cast<WheelDirection>(wheelDirection),
            controlDeck, consoleVariable);
    }

    return nullptr;
}

std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
AxisDirectionMappingFactory::CreateDefaultKeyboardAxisDirectionMappings(
    uint8_t portIndex, StickIndex stickIndex, std::shared_ptr<ConsoleVariable> consoleVariable,
    std::shared_ptr<ControlDeck> controlDeck, std::shared_ptr<Window> window) {
    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings;

    auto defaultsForStick =
        controlDeck->GetControllerDefaultMappings()->GetDefaultKeyboardKeyToAxisDirectionMappings()[stickIndex];

    for (const auto& [direction, scancode] : defaultsForStick) {
        mappings.push_back(std::make_shared<KeyboardKeyToAxisDirectionMapping>(
            portIndex, stickIndex, direction, scancode, controlDeck, window, consoleVariable));
    }

    return mappings;
}

std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
AxisDirectionMappingFactory::CreateDefaultSDLAxisDirectionMappings(uint8_t portIndex, StickIndex stickIndex,
                                                                   std::shared_ptr<ConsoleVariable> consoleVariable,
                                                                   std::shared_ptr<ControlDeck> controlDeck) {
    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings;

    auto defaultButtonsForStick =
        controlDeck->GetControllerDefaultMappings()->GetDefaultSDLButtonToAxisDirectionMappings()[stickIndex];

    for (const auto& [direction, sdlGamepadButton] : defaultButtonsForStick) {
        mappings.push_back(std::make_shared<SDLButtonToAxisDirectionMapping>(
            portIndex, stickIndex, direction, sdlGamepadButton, controlDeck, consoleVariable));
    }

    auto defaultAxisDirectionsForStick =
        controlDeck->GetControllerDefaultMappings()->GetDefaultSDLAxisDirectionToAxisDirectionMappings()[stickIndex];

    for (const auto& [direction, sdlGamepadAxisDirection] : defaultAxisDirectionsForStick) {
        auto [sdlGamepadAxis, sdlGamepadDirection] = sdlGamepadAxisDirection;
        mappings.push_back(std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(
            portIndex, stickIndex, direction, sdlGamepadAxis, sdlGamepadDirection, controlDeck, consoleVariable));
    }

    return mappings;
}

std::shared_ptr<ControllerAxisDirectionMapping> AxisDirectionMappingFactory::CreateAxisDirectionMappingFromSDLInput(
    uint8_t portIndex, StickIndex stickIndex, Direction direction, std::shared_ptr<ConsoleVariable> consoleVariable,
    std::shared_ptr<ControlDeck> controlDeck) {
    std::shared_ptr<ControllerAxisDirectionMapping> mapping = nullptr;

    for (auto [instanceId, gamepad] :
         controlDeck->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(portIndex)) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLButtonToAxisDirectionMapping>(portIndex, stickIndex, direction, button,
                                                                            controlDeck, consoleVariable);
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

            mapping = std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(
                portIndex, stickIndex, direction, axis, axisDirection, controlDeck, consoleVariable);
            break;
        }
    }

    return mapping;
}

std::shared_ptr<ControllerAxisDirectionMapping>
AxisDirectionMappingFactory::CreateAxisDirectionMappingFromMouseWheelInput(
    uint8_t portIndex, StickIndex stickIndex, Direction direction, std::shared_ptr<ConsoleVariable> consoleVariable,
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

    return std::make_shared<MouseWheelToAxisDirectionMapping>(portIndex, stickIndex, direction, wheelDirection,
                                                              controlDeck, consoleVariable);
}
} // namespace Ship
