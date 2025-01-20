#include "AxisDirectionMappingFactory.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/mouse/MouseButtonToAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/mouse/MouseWheelToAxisDirectionMapping.h"

#include "controller/controldevice/controller/mapping/sdl/SDLButtonToAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.h"

#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"
#include "Context.h"

#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "controller/controldevice/controller/mapping/mouse/WheelHandler.h"

namespace Ship {
std::shared_ptr<ControllerAxisDirectionMapping>
AxisDirectionMappingFactory::CreateAxisDirectionMappingFromConfig(uint8_t portIndex, StickIndex stickIndex,
                                                                  std::string id) {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "SDLAxisDirectionToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), -1);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) ||
            sdlControllerAxis == -1 || (axisDirection != NEGATIVE && axisDirection != POSITIVE)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), sdlControllerAxis, axisDirection);
    }

    if (mappingClass == "SDLButtonToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) ||
            sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLButtonToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), sdlControllerButton);
    }

    if (mappingClass == "KeyboardKeyToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t scancode =
            CVarGetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), 0);

        if (direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<KeyboardKeyToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), static_cast<KbScancode>(scancode));
    }

    if (mappingClass == "MouseButtonToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int mouseButton = CVarGetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), 0);

        if (direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<MouseButtonToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), static_cast<MouseBtn>(mouseButton));
    }

    if (mappingClass == "MouseWheelToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int wheelDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<MouseWheelToAxisDirectionMapping>(
            portIndex, stickIndex, static_cast<Direction>(direction), static_cast<WheelDirection>(wheelDirection));
    }

    return nullptr;
}

std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
AxisDirectionMappingFactory::CreateDefaultKeyboardAxisDirectionMappings(uint8_t portIndex, StickIndex stickIndex) {
    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings;

    auto defaultsForStick = Context::GetInstance()
                                ->GetControlDeck()
                                ->GetControllerDefaultMappings()
                                ->GetDefaultKeyboardKeyToAxisDirectionMappings()[stickIndex];

    for (const auto& [direction, scancode] : defaultsForStick) {
        mappings.push_back(
            std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stickIndex, direction, scancode));
    }

    return mappings;
}

std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
AxisDirectionMappingFactory::CreateDefaultSDLAxisDirectionMappings(uint8_t portIndex, StickIndex stickIndex) {
    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings;

    auto defaultButtonsForStick = Context::GetInstance()
                                      ->GetControlDeck()
                                      ->GetControllerDefaultMappings()
                                      ->GetDefaultSDLButtonToAxisDirectionMappings()[stickIndex];

    for (const auto& [direction, sdlGamepadButton] : defaultButtonsForStick) {
        mappings.push_back(
            std::make_shared<SDLButtonToAxisDirectionMapping>(portIndex, stickIndex, direction, sdlGamepadButton));
    }

    auto defaultAxisDirectionsForStick = Context::GetInstance()
                                             ->GetControlDeck()
                                             ->GetControllerDefaultMappings()
                                             ->GetDefaultSDLAxisDirectionToAxisDirectionMappings()[stickIndex];

    for (const auto& [direction, sdlGamepadAxisDirection] : defaultAxisDirectionsForStick) {
        auto [sdlGamepadAxis, sdlGamepadDirection] = sdlGamepadAxisDirection;
        mappings.push_back(std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(
            portIndex, stickIndex, direction, sdlGamepadAxis, sdlGamepadDirection));
    }

    return mappings;
}

std::shared_ptr<ControllerAxisDirectionMapping>
AxisDirectionMappingFactory::CreateAxisDirectionMappingFromSDLInput(uint8_t portIndex, StickIndex stickIndex,
                                                                    Direction direction) {
    std::shared_ptr<ControllerAxisDirectionMapping> mapping = nullptr;

    for (auto [instanceId, gamepad] :
         Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(
             portIndex)) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLButtonToAxisDirectionMapping>(portIndex, stickIndex, direction, button);
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

            mapping = std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(portIndex, stickIndex, direction, axis,
                                                                               axisDirection);
            break;
        }
    }

    return mapping;
}

std::shared_ptr<ControllerAxisDirectionMapping>
AxisDirectionMappingFactory::CreateAxisDirectionMappingFromMouseWheelInput(uint8_t portIndex, StickIndex stickIndex,
                                                                           Direction direction) {
    WheelDirections wheelDirections = WheelHandler::GetInstance()->GetDirections();
    WheelDirection wheelDirection;
    if (wheelDirections.x != LUS_WHEEL_NONE) {
        wheelDirection = wheelDirections.x;
    } else if (wheelDirections.y != LUS_WHEEL_NONE) {
        wheelDirection = wheelDirections.y;
    } else {
        return nullptr;
    }

    return std::make_shared<MouseWheelToAxisDirectionMapping>(portIndex, stickIndex, direction, wheelDirection);
}
} // namespace Ship
