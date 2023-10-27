#include "AxisDirectionMappingFactory.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h"

#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/wiiu/WiiUButtonToAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/wiiu/WiiUAxisDirectionToAxisDirectionMapping.h"
#else
#include "controller/controldevice/controller/mapping/sdl/SDLButtonToAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.h"
#endif

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h"

namespace LUS {
std::shared_ptr<ControllerAxisDirectionMapping>
AxisDirectionMappingFactory::CreateAxisDirectionMappingFromConfig(uint8_t portIndex, Stick stick, std::string id) {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(), "");

#ifdef __WIIU__
// todo
#else
    if (mappingClass == "SDLAxisDirectionToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), -1);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) || lusDeviceIndex < 0 ||
            sdlControllerAxis == -1 || (axisDirection != NEGATIVE && axisDirection != POSITIVE)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(
            static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex, stick, static_cast<Direction>(direction),
            sdlControllerAxis, axisDirection);
    }

    if (mappingClass == "SDLButtonToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) || lusDeviceIndex < 0 ||
            sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLButtonToAxisDirectionMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex,
                                                                 stick, static_cast<Direction>(direction),
                                                                 sdlControllerButton);
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

        return std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, static_cast<Direction>(direction),
                                                                   static_cast<KbScancode>(scancode));
    }
#endif

    return nullptr;
}

#ifdef __WIIU__
// todo
#else
std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
AxisDirectionMappingFactory::CreateDefaultKeyboardAxisDirectionMappings(uint8_t portIndex, Stick stick) {
    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings = {
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, LEFT, LUS_KB_A),
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, RIGHT, LUS_KB_D),
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, UP, LUS_KB_W),
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, DOWN, LUS_KB_S)
    };

    return mappings;
}

#ifdef __WIIU__
// todo
#else
std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>
AxisDirectionMappingFactory::CreateDefaultSDLAxisDirectionMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                                                   Stick stick) {
    auto sdlIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(
        Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(lusDeviceIndex));
    if (sdlIndexMapping == nullptr) {
        return std::vector<std::shared_ptr<ControllerAxisDirectionMapping>>();
    }

    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings = {
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(lusDeviceIndex, portIndex, stick, LEFT,
                                                                 stick == LEFT_STICK ? 0 : 2, -1),
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(lusDeviceIndex, portIndex, stick, RIGHT,
                                                                 stick == LEFT_STICK ? 0 : 2, 1),
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(lusDeviceIndex, portIndex, stick, UP,
                                                                 stick == LEFT_STICK ? 1 : 3, -1),
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(lusDeviceIndex, portIndex, stick, DOWN,
                                                                 stick == LEFT_STICK ? 1 : 3, 1)
    };

    return mappings;
}
#endif

std::shared_ptr<ControllerAxisDirectionMapping>
AxisDirectionMappingFactory::CreateAxisDirectionMappingFromSDLInput(uint8_t portIndex, Stick stick,
                                                                    Direction direction) {
    std::unordered_map<LUSDeviceIndex, SDL_GameController*> sdlControllers;
    std::shared_ptr<ControllerAxisDirectionMapping> mapping = nullptr;
    for (auto [lusIndex, indexMapping] :
         Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetAllDeviceIndexMappings()) {
        auto sdlIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(indexMapping);

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
                mapping =
                    std::make_shared<SDLButtonToAxisDirectionMapping>(lusIndex, portIndex, stick, direction, button);
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

            mapping = std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(lusIndex, portIndex, stick, direction,
                                                                               axis, axisDirection);
            break;
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
#endif
} // namespace LUS
