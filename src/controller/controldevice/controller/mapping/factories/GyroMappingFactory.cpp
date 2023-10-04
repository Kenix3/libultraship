#include "GyroMappingFactory.h"
#include "controller/controldevice/controller/mapping/sdl/SDLGyroMapping.h"
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"

namespace LUS {
std::shared_ptr<ControllerGyroMapping> GyroMappingFactory::CreateGyroMappingFromConfig(uint8_t portIndex,
                                                                                       std::string id) {
    const std::string mappingCvarKey = "gControllers.GyroMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str(), "");

    float sensitivity = CVarGetFloat(StringHelper::Sprintf("%s.Sensitivity", mappingCvarKey.c_str()).c_str(), 2.0f);
    if (sensitivity < 0.0f || sensitivity > 1.0f) {
        // something about this mapping is invalid
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return nullptr;
    }

    if (mappingClass == "SDLGyroMapping") {
        int32_t sdlControllerIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        if (sdlControllerIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        float neutralPitch =
            CVarGetFloat(StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralYaw = CVarGetFloat(StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralRoll = CVarGetFloat(StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str(), 0.0f);

        return std::make_shared<SDLGyroMapping>(portIndex, sensitivity, neutralPitch, neutralYaw, neutralRoll,
                                                sdlControllerIndex);
    }

    return nullptr;
}

std::shared_ptr<ControllerGyroMapping> GyroMappingFactory::CreateGyroMappingFromSDLInput(uint8_t portIndex) {
    std::unordered_map<int32_t, SDL_GameController*> sdlControllersWithGyro;
    std::shared_ptr<ControllerGyroMapping> mapping = nullptr;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            auto controller = SDL_GameControllerOpen(i);
            if (SDL_GameControllerHasSensor(controller, SDL_SENSOR_GYRO)) {
                sdlControllersWithGyro[i] = SDL_GameControllerOpen(i);
            } else {
                SDL_GameControllerClose(controller);
            }
        }
    }

    for (auto [controllerIndex, controller] : sdlControllersWithGyro) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLGyroMapping>(portIndex, 1.0f, 0.0f, 0.0f, 0.0f, controllerIndex);
                mapping->Recalibrate();
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

            mapping = std::make_shared<SDLGyroMapping>(portIndex, 1.0f, 0.0f, 0.0f, 0.0f, controllerIndex);
            mapping->Recalibrate();
            break;
        }
    }

    for (auto [i, controller] : sdlControllersWithGyro) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
} // namespace LUS
