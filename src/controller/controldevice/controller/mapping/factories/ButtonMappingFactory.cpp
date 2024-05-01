#include "ButtonMappingFactory.h"
#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"
#include "libultraship/libultra/controller.h"
#include "Context.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#include "controller/deviceindex/ShipDeviceIndexToSDLDeviceIndexMapping.h"

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

    return nullptr;
}

std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultKeyboardButtonMappings(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    switch (bitmask) {
        case BTN_A:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_A, KbScancode::LUS_KB_X));
            break;
        case BTN_B:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_B, KbScancode::LUS_KB_C));
            break;
        case BTN_L:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_L, KbScancode::LUS_KB_E));
            break;
        case BTN_R:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_R, KbScancode::LUS_KB_R));
            break;
        case BTN_Z:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_Z, KbScancode::LUS_KB_Z));
            break;
        case BTN_START:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_START, KbScancode::LUS_KB_SPACE));
            break;
        case BTN_CUP:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CUP, KbScancode::LUS_KB_ARROWKEY_UP));
            break;
        case BTN_CDOWN:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CDOWN, KbScancode::LUS_KB_ARROWKEY_DOWN));
            break;
        case BTN_CLEFT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CLEFT, KbScancode::LUS_KB_ARROWKEY_LEFT));
            break;
        case BTN_CRIGHT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CRIGHT, KbScancode::LUS_KB_ARROWKEY_RIGHT));
            break;
        case BTN_DUP:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DUP, KbScancode::LUS_KB_T));
            break;
        case BTN_DDOWN:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DDOWN, KbScancode::LUS_KB_G));
            break;
        case BTN_DLEFT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DLEFT, KbScancode::LUS_KB_F));
            break;
        case BTN_DRIGHT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DRIGHT, KbScancode::LUS_KB_H));
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
                std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_A, SDL_CONTROLLER_BUTTON_A));
            break;
        case BTN_B:
            mappings.push_back(
                std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_B, SDL_CONTROLLER_BUTTON_B));
            break;
        case BTN_L:
            if (!isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_L,
                                                                              SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
            }
            break;
        case BTN_R:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_R,
                                                                                 SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1));
            break;
        case BTN_Z:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_Z,
                                                                                 SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1));
            break;
        case BTN_START:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_START,
                                                                          SDL_CONTROLLER_BUTTON_START));
            break;
        case BTN_CUP:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CUP,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTY, -1));
            break;
        case BTN_CDOWN:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CDOWN,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTY, 1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_CDOWN,
                                                                              SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
            }
            break;
        case BTN_CLEFT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CLEFT,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTX, -1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_CLEFT,
                                                                              SDL_CONTROLLER_BUTTON_Y));
            }
            break;
        case BTN_CRIGHT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(shipDeviceIndex, portIndex, BTN_CRIGHT,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTX, 1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_CRIGHT,
                                                                              SDL_CONTROLLER_BUTTON_X));
            }
            break;
        case BTN_DUP:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DUP,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_UP));
            break;
        case BTN_DDOWN:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DDOWN,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_DOWN));
            break;
        case BTN_DLEFT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DLEFT,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_LEFT));
            break;
        case BTN_DRIGHT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(shipDeviceIndex, portIndex, BTN_DRIGHT,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
            break;
    }

    return mappings;
}

std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromSDLInput(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask) {
    std::unordered_map<ShipDeviceIndex, SDL_GameController*> sdlControllers;
    std::shared_ptr<ControllerButtonMapping> mapping = nullptr;
    for (auto [lusIndex, indexMapping] :
         Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetAllDeviceIndexMappings()) {
        auto sdlIndexMapping = std::dynamic_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(indexMapping);

        if (sdlIndexMapping == nullptr) {
            // this LUS index isn't mapped to an SDL index
            continue;
        }

        auto sdlIndex = sdlIndexMapping->GetSDLDeviceIndex();

        if (!SDL_IsGameController(sdlIndex)) {
            // this SDL device isn't a game controller
            continue;
        }

        sdlControllers[lusIndex] = SDL_GameControllerOpen(sdlIndex);
    }

    for (auto [lusIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLButtonToButtonMapping>(lusIndex, portIndex, bitmask, button);
                break;
            }
        }

        if (mapping != nullptr) {
            break;
        }

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            const auto axis = static_cast<SDL_GameControllerAxis>(i);
            const auto axisValue = SDL_GameControllerGetAxis(controller, axis) / 32767.0f;
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
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
} // namespace Ship
