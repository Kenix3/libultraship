#include "ButtonMappingFactory.h"
#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"
#include "libultraship/libultra/controller.h"
#include "Context.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"
#include "controller/controldevice/controller/mapping/mouse/MouseButtonToButtonMapping.h"
#include "controller/controldevice/controller/mapping/mouse/MouseWheelToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "controller/controldevice/controller/mapping/mouse/WheelHandler.h"

namespace Ship {
std::shared_ptr<ControllerButtonMapping> ButtonMappingFactory::CreateButtonMappingFromConfig(uint8_t portIndex,
                                                                                             std::string id) {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".ButtonMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "");
    CONTROLLERBUTTONS_T bitmask =
        CVarGetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), 0);
    if (!bitmask) {
        // all button mappings need bitmasks
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return nullptr;
    }

    if (mappingClass == "SDLButtonToButtonMapping") {
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if (sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, sdlControllerButton);
    }

    if (mappingClass == "SDLAxisDirectionToButtonMapping") {
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), -1);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerAxis == -1 || (axisDirection != -1 && axisDirection != 1)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, bitmask, sdlControllerAxis, axisDirection);
    }

    if (mappingClass == "KeyboardKeyToButtonMapping") {
        int32_t scancode =
            CVarGetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<KeyboardKeyToButtonMapping>(portIndex, bitmask, static_cast<KbScancode>(scancode));
    }

    if (mappingClass == "MouseButtonToButtonMapping") {
        int mouseButton = CVarGetInteger(StringHelper::Sprintf("%s.MouseButton", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<MouseButtonToButtonMapping>(portIndex, bitmask, static_cast<MouseBtn>(mouseButton));
    }

    if (mappingClass == "MouseWheelToButtonMapping") {
        int wheelDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.WheelDirection", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<MouseWheelToButtonMapping>(portIndex, bitmask,
                                                           static_cast<WheelDirection>(wheelDirection));
    }

    return nullptr;
}

std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultKeyboardButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    auto defaultsForBitmask = Context::GetInstance()
                                  ->GetControlDeck()
                                  ->GetControllerDefaultMappings()
                                  ->GetDefaultKeyboardKeyToButtonMappings()[bitmask];

    for (const auto& scancode : defaultsForBitmask) {
        mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, bitmask, scancode));
    }

    return mappings;
}

std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultSDLButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    auto defaultButtonsForBitmask = Context::GetInstance()
                                        ->GetControlDeck()
                                        ->GetControllerDefaultMappings()
                                        ->GetDefaultSDLButtonToButtonMappings()[bitmask];

    for (const auto& sdlGamepadButton : defaultButtonsForBitmask) {
        mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, sdlGamepadButton));
    }

    auto defaultAxisDirectionsForBitmask = Context::GetInstance()
                                               ->GetControlDeck()
                                               ->GetControllerDefaultMappings()
                                               ->GetDefaultSDLAxisDirectionToButtonMappings()[bitmask];

    for (const auto& [sdlGamepadAxis, axisDirection] : defaultAxisDirectionsForBitmask) {
        mappings.push_back(
            std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, bitmask, sdlGamepadAxis, axisDirection));
    }

    return mappings;
}

std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromSDLInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask) {
    std::shared_ptr<ControllerButtonMapping> mapping = nullptr;

    for (auto [instanceId, gamepad] :
         Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(
             portIndex)) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, button);
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

            mapping = std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, bitmask, axis, axisDirection);
            break;
        }
    }

    return mapping;
}

std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromMouseWheelInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask) {
    WheelDirections wheelDirections = WheelHandler::GetInstance()->GetDirections();
    WheelDirection wheelDirection;
    if (wheelDirections.x != LUS_WHEEL_NONE) {
        wheelDirection = wheelDirections.x;
    } else if (wheelDirections.y != LUS_WHEEL_NONE) {
        wheelDirection = wheelDirections.y;
    } else {
        return nullptr;
    }

    return std::make_shared<MouseWheelToButtonMapping>(portIndex, bitmask, wheelDirection);
}
} // namespace Ship
