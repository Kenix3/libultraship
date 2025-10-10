#include "ship/controller/controldevice/controller/mapping/factories/GyroMappingFactory.h"
#include "ship/controller/controldevice/controller/mapping/sdl/SDLGyroMapping.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/utils/StringHelper.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
std::shared_ptr<ControllerGyroMapping> GyroMappingFactory::CreateGyroMappingFromConfig(uint8_t portIndex,
                                                                                       std::string id) {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GyroMappings." + id;
    const std::string mappingClass = Ship::Context::GetInstance()->GetConsoleVariables()->GetString(
        StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str(), "");

    float sensitivity = Ship::Context::GetInstance()->GetConsoleVariables()->GetFloat(
        StringHelper::Sprintf("%s.Sensitivity", mappingCvarKey.c_str()).c_str(), 2.0f);
    if (sensitivity < 0.0f || sensitivity > 1.0f) {
        // something about this mapping is invalid
        Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(mappingCvarKey.c_str());
        Ship::Context::GetInstance()->GetConsoleVariables()->Save();
        return nullptr;
    }

    if (mappingClass == "SDLGyroMapping") {
        float neutralPitch = Ship::Context::GetInstance()->GetConsoleVariables()->GetFloat(
            StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralYaw = Ship::Context::GetInstance()->GetConsoleVariables()->GetFloat(
            StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralRoll = Ship::Context::GetInstance()->GetConsoleVariables()->GetFloat(
            StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str(), 0.0f);

        return std::make_shared<SDLGyroMapping>(portIndex, sensitivity, neutralPitch, neutralYaw, neutralRoll);
    }

    return nullptr;
}

std::shared_ptr<ControllerGyroMapping> GyroMappingFactory::CreateGyroMappingFromSDLInput(uint8_t portIndex) {
    std::shared_ptr<ControllerGyroMapping> mapping = nullptr;

    for (auto [instanceId, gamepad] :
         Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(
             portIndex)) {
        if (!SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_GYRO)) {
            continue;
        }

        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLGyroMapping>(portIndex, 1.0f, 0.0f, 0.0f, 0.0f);
                mapping->Recalibrate();
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

            mapping = std::make_shared<SDLGyroMapping>(portIndex, 1.0f, 0.0f, 0.0f, 0.0f);
            mapping->Recalibrate();
            break;
        }
    }

    return mapping;
}
} // namespace Ship
