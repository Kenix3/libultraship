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
#include "controller/deviceindex/ShipDeviceIndexToSDLDeviceIndexMapping.h"
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
        int32_t shipDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if (shipDeviceIndex < 0 || sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLButtonToButtonMapping>(static_cast<ShipDeviceIndex>(shipDeviceIndex), portIndex,
                                                          bitmask, sdlControllerButton);
    }

    if (mappingClass == "SDLAxisDirectionToButtonMapping") {
        int32_t shipDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), -1);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (shipDeviceIndex < 0 || sdlControllerAxis == -1 || (axisDirection != -1 && axisDirection != 1)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToButtonMapping>(static_cast<ShipDeviceIndex>(shipDeviceIndex),
                                                                 portIndex, bitmask, sdlControllerAxis, axisDirection);
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

    switch (bitmask) {
        case BTN_A:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_A, LUS_DEFAULT_KB_MAPPING_A));
            break;
        case BTN_B:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_B, LUS_DEFAULT_KB_MAPPING_B));
            break;
        case BTN_L:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_L, LUS_DEFAULT_KB_MAPPING_L));
            break;
        case BTN_R:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_R, LUS_DEFAULT_KB_MAPPING_R));
            break;
        case BTN_Z:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_Z, LUS_DEFAULT_KB_MAPPING_Z));
            break;
        case BTN_START:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_START, LUS_DEFAULT_KB_MAPPING_START));
            break;
        case BTN_CUP:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CUP, LUS_DEFAULT_KB_MAPPING_CUP));
            break;
        case BTN_CDOWN:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CDOWN, LUS_DEFAULT_KB_MAPPING_CDOWN));
            break;
        case BTN_CLEFT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CLEFT, LUS_DEFAULT_KB_MAPPING_CLEFT));
            break;
        case BTN_CRIGHT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CRIGHT, LUS_DEFAULT_KB_MAPPING_CRIGHT));
            break;
        case BTN_DUP:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DUP, LUS_DEFAULT_KB_MAPPING_DUP));
            break;
        case BTN_DDOWN:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DDOWN, LUS_DEFAULT_KB_MAPPING_DDOWN));
            break;
        case BTN_DLEFT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DLEFT, LUS_DEFAULT_KB_MAPPING_DLEFT));
            break;
        case BTN_DRIGHT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DRIGHT, LUS_DEFAULT_KB_MAPPING_DRIGHT));
            break;
    }

    return mappings;
}

std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultSDLButtonMappings(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex,
                                                     CONTROLLERBUTTONS_T bitmask) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    auto sdlIndexMapping = std::dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(
        Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromShipDeviceIndex(shipDeviceIndex));
    if (sdlIndexMapping == nullptr) {
        return std::vector<std::shared_ptr<ControllerButtonMapping>>();
    }

    bool isGameCube = sdlIndexMapping->GetSDLControllerName() == "Nintendo GameCube Controller";

    switch (bitmask) {
        case BTN_A:
            mappings.push_back(
                std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_A, SDL_GAMEPAD_BUTTON_SOUTH));
            break;
        case BTN_B:
            mappings.push_back(
                std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_B, SDL_GAMEPAD_BUTTON_EAST));
            break;
        case BTN_L:
            if (!isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_L,
                                                                              SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
            }
            break;
        case BTN_R:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_R,
                                                                                 SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 1));
            break;
        case BTN_Z:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_Z,
                                                                                 SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 1));
            break;
        case BTN_START:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_START,
                                                                          SDL_GAMEPAD_BUTTON_START));
            break;
        case BTN_CUP:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CUP,
                                                                                 SDL_GAMEPAD_AXIS_RIGHTY, -1));
            break;
        case BTN_CDOWN:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CDOWN,
                                                                                 SDL_GAMEPAD_AXIS_RIGHTY, 1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_CDOWN,
                                                                              SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
            }
            break;
        case BTN_CLEFT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CLEFT,
                                                                                 SDL_GAMEPAD_AXIS_RIGHTX, -1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_CLEFT,
                                                                              SDL_GAMEPAD_BUTTON_NORTH));
            }
            break;
        case BTN_CRIGHT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CRIGHT,
                                                                                 SDL_GAMEPAD_AXIS_RIGHTX, 1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_CRIGHT,
                                                                              SDL_GAMEPAD_BUTTON_WEST));
            }
            break;
        case BTN_DUP:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DUP,
                                                                          SDL_GAMEPAD_BUTTON_DPAD_UP));
            break;
        case BTN_DDOWN:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DDOWN,
                                                                          SDL_GAMEPAD_BUTTON_DPAD_DOWN));
            break;
        case BTN_DLEFT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DLEFT,
                                                                          SDL_GAMEPAD_BUTTON_DPAD_LEFT));
            break;
        case BTN_DRIGHT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DRIGHT,
                                                                          SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
            break;
    }

    return mappings;
}

std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromSDLInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask) {
    std::unordered_map<ShipDeviceIndex, SDL_Gamepad*> sdlControllers;
    std::shared_ptr<ControllerButtonMapping> mapping = nullptr;
    for (auto [lusIndex, indexMapping] :
         Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetAllDeviceIndexMappings()) {
        auto sdlIndexMapping = std::dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(indexMapping);

        if (sdlIndexMapping == nullptr) {
            // this LUS index isn't mapped to an SDL index
            continue;
        }

        auto sdlIndex = sdlIndexMapping->GetSDLDeviceIndex();

        if (!SDL_IsGamepad(sdlIndex)) {
            // this SDL device isn't a game controller
            continue;
        }

        sdlControllers[lusIndex] = SDL_OpenGamepad(sdlIndex);
    }

    for (auto [lusIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_GAMEPAD_BUTTON_SOUTH; button < SDL_GAMEPAD_BUTTON_COUNT; button++) {
            if (SDL_GetGamepadButton(controller, static_cast<SDL_GamepadButton>(button))) {
                mapping = std::make_shared<SDLButtonToButtonMapping>(lusIndex, portIndex, bitmask, button);
                break;
            }
        }

        if (mapping != nullptr) {
            break;
        }

        for (int32_t i = SDL_GAMEPAD_AXIS_LEFTX; i < SDL_GAMEPAD_AXIS_COUNT; i++) {
            const auto axis = static_cast<SDL_GamepadAxis>(i);
            const auto axisValue = SDL_GetGamepadAxis(controller, axis) / 32767.0f;
            int32_t axisDirection = 0;
            if (axisValue < -0.7f) {
                axisDirection = NEGATIVE;
            } else if (axisValue > 0.7f) {
                axisDirection = POSITIVE;
            }

            if (axisDirection == 0) {
                continue;
            }

            mapping =
                std::make_shared<SDLAxisDirectionToButtonMapping>(lusIndex, portIndex, bitmask, axis, axisDirection);
            break;
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_CloseGamepad(controller);
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
