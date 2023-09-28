#include "ButtonMappingFactory.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"

namespace LUS {
std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromConfig(uint8_t portIndex, std::string id) {
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "");
    uint16_t bitmask = CVarGetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), 0);
    if (!bitmask) {
        // all button mappings need bitmasks
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return nullptr;
    }

    if (mappingClass == "SDLButtonToButtonMapping") {
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerIndex < 0 || sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, sdlControllerIndex, sdlControllerButton);
    }

    if (mappingClass == "SDLAxisDirectionToButtonMapping") {
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), 0);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (sdlControllerIndex < 0 || sdlControllerAxis == -1 || (axisDirection != -1 && axisDirection != 1)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, bitmask, sdlControllerIndex,
                                                                           sdlControllerAxis, axisDirection);
    }

    if (mappingClass == "KeyboardKeyToButtonMapping") {
        int32_t scancode =
            CVarGetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<KeyboardKeyToButtonMapping>(portIndex, bitmask, static_cast<KbScancode>(scancode));
    }

    return nullptr;
}

std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultKeyboardButtonMappings(uint8_t portIndex, uint16_t bitmask) {
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
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_CRIGHT,
                                                                          KbScancode::LUS_KB_ARROWKEY_RIGHT));
            break;
        case BTN_DUP:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DUP, KbScancode::LUS_KB_T));
            break;
        case BTN_DDOWN:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DDOWN, KbScancode::LUS_KB_G));
            break;
        case BTN_DLEFT:
            mappings.push_back(std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DLEFT, KbScancode::LUS_KB_F));
            break;
        case BTN_DRIGHT:
            mappings.push_back(
                std::make_shared<KeyboardKeyToButtonMapping>(portIndex, BTN_DRIGHT, KbScancode::LUS_KB_H));
            break;
    }

    return mappings;
}

std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultSDLButtonMappings(uint8_t portIndex, uint16_t bitmask, int32_t sdlControllerIndex) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    switch (bitmask) {
        case BTN_A:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_A, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_A));
            break;
        case BTN_B:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_B, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_B));
            break;
        case BTN_L:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_L, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
            break;
        case BTN_R:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, BTN_R, sdlControllerIndex,
                                                                               SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1));
            break;
        case BTN_Z:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, BTN_Z, sdlControllerIndex,
                                                                               SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1));
            break;
        case BTN_START:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_START, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_START));
            break;
        case BTN_CUP:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, BTN_CUP, sdlControllerIndex,
                                                                               SDL_CONTROLLER_AXIS_RIGHTY, -1));
            break;
        case BTN_CDOWN:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(
                portIndex, BTN_CDOWN, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTY, 1));
            break;
        case BTN_CLEFT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(
                portIndex, BTN_CLEFT, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTX, -1));
            break;
        case BTN_CRIGHT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(
                portIndex, BTN_CRIGHT, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTX, 1));
            break;
        case BTN_DUP:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_DUP, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_DPAD_UP));
            break;
        case BTN_DDOWN:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_DDOWN, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_DPAD_DOWN));
            break;
        case BTN_DLEFT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_DLEFT, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_DPAD_LEFT));
            break;
        case BTN_DRIGHT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(portIndex, BTN_DRIGHT, sdlControllerIndex,
                                                                        SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
            break;
    }

    return mappings;
}

std::shared_ptr<ControllerButtonMapping>
ButtonMappingFactory::CreateButtonMappingFromRawPress(uint8_t portIndex, uint16_t bitmask) {
    // sdl
    std::unordered_map<int32_t, SDL_GameController*> sdlControllers;
    std::shared_ptr<ControllerButtonMapping> mapping = nullptr;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sdlControllers[i] = SDL_GameControllerOpen(i);
        }
    }

    for (auto [controllerIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLButtonToButtonMapping>(portIndex, bitmask, controllerIndex, button);
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

            mapping = std::make_shared<SDLAxisDirectionToButtonMapping>(portIndex, bitmask, controllerIndex, axis,
                                                                             axisDirection);
            break;
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
} // namespace LUS