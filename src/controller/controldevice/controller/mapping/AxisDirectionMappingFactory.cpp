#include "AxisDirectionMappingFactory.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLButtonToAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToAxisDirectionMapping.h"
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
std::shared_ptr<ControllerAxisDirectionMapping> AxisDirectionMappingFactory::CreateAxisDirectionMappingFromConfig(uint8_t portIndex, Stick stick, std::string id) {
    const std::string mappingCvarKey = "gControllers.AxisDirectionMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(), "");

    if (mappingClass == "SDLAxisDirectionToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), 0);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) || sdlControllerIndex < 0 || sdlControllerAxis == -1 ||
            (axisDirection != NEGATIVE && axisDirection != POSITIVE)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(portIndex, stick, static_cast<Direction>(direction), sdlControllerIndex, sdlControllerAxis, axisDirection);
    }

    if (mappingClass == "SDLAxisDirectionToAxisDirectionMapping") {
        int32_t direction = CVarGetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerIndex", mappingCvarKey.c_str()).c_str(), 0);
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), 0);

        if ((direction != LEFT && direction != RIGHT && direction != UP && direction != DOWN) || sdlControllerIndex < 0 || sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLButtonToAxisDirectionMapping>(portIndex, stick, static_cast<Direction>(direction), sdlControllerIndex, sdlControllerButton);
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

        return std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, static_cast<Direction>(direction), static_cast<KbScancode>(scancode));
    }
}

std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> AxisDirectionMappingFactory::CreateDefaultKeyboardAxisDirectionMappings(uint8_t portIndex, Stick stick) {
    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings = {
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, LEFT, LUS_KB_A),
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, RIGHT, LUS_KB_D),
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, UP, LUS_KB_W),
        std::make_shared<KeyboardKeyToAxisDirectionMapping>(portIndex, stick, DOWN, LUS_KB_S)
    };

    return mappings;
}

std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> AxisDirectionMappingFactory::CreateDefaultSDLAxisDirectionMappings(uint8_t portIndex, Stick stick, int32_t sdlControllerIndex) {
    std::vector<std::shared_ptr<ControllerAxisDirectionMapping>> mappings = {
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(portIndex, stick, LEFT, sdlControllerIndex, stick == LEFT_STICK ? 0 : 2, -1),
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(portIndex, stick, RIGHT, sdlControllerIndex, stick == LEFT_STICK ? 0 : 2, 1),
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(portIndex, stick, UP, sdlControllerIndex, stick == LEFT_STICK ? 1 : 3, -1),
        std::make_shared<SDLAxisDirectionToAxisDirectionMapping>(portIndex, stick, DOWN, sdlControllerIndex, stick == LEFT_STICK ? 1 : 3, 1)
    };

    return mappings;
}
}