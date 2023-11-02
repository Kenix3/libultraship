#include "GyroMappingFactory.h"
#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/wiiu/WiiUGyroMapping.h"
#else
#include "controller/controldevice/controller/mapping/sdl/SDLGyroMapping.h"
#endif
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h"

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
#ifdef __WIIU__
    if (mappingClass == "WiiUGyroMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        float neutralPitch =
            CVarGetFloat(StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralYaw = CVarGetFloat(StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralRoll = CVarGetFloat(StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str(), 0.0f);

        return std::make_shared<WiiUGyroMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex, sensitivity,
                                                 neutralPitch, neutralYaw, neutralRoll);
    }
#else
    if (mappingClass == "SDLGyroMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        float neutralPitch =
            CVarGetFloat(StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralYaw = CVarGetFloat(StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str(), 0.0f);
        float neutralRoll = CVarGetFloat(StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str(), 0.0f);

        return std::make_shared<SDLGyroMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex, sensitivity,
                                                neutralPitch, neutralYaw, neutralRoll);
    }
#endif
    return nullptr;
}

#ifdef __WIIU__
std::shared_ptr<ControllerGyroMapping> GyroMappingFactory::CreateGyroMappingFromWiiUInput(uint8_t portIndex) {
    for (auto [lusIndex, indexMapping] :
         Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetAllDeviceIndexMappings()) {
        auto wiiuIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToWiiUDeviceIndexMapping>(indexMapping);

        if (wiiuIndexMapping == nullptr) {
            continue;
        }

        if (wiiuIndexMapping->IsWiiUGamepad()) {
            VPADReadError verror;
            VPADStatus* vstatus = LUS::WiiU::GetVPADStatus(&verror);

            if (vstatus == nullptr || verror != VPAD_READ_SUCCESS) {
                continue;
            }

            for (uint32_t i = VPAD_BUTTON_SYNC; i <= VPAD_STICK_L_EMULATION_LEFT; i <<= 1) {
                if (!(vstatus->hold & i)) {
                    continue;
                }

                return std::make_shared<WiiUGyroMapping>(lusIndex, portIndex, 1.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    }

    return nullptr;
}
#else
std::shared_ptr<ControllerGyroMapping> GyroMappingFactory::CreateGyroMappingFromSDLInput(uint8_t portIndex) {
    std::unordered_map<LUSDeviceIndex, SDL_GameController*> sdlControllersWithGyro;
    std::shared_ptr<ControllerGyroMapping> mapping = nullptr;
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

        auto controller = SDL_GameControllerOpen(sdlIndex);
        if (SDL_GameControllerHasSensor(controller, SDL_SENSOR_GYRO)) {
            sdlControllersWithGyro[lusIndex] = SDL_GameControllerOpen(sdlIndex);
        } else {
            SDL_GameControllerClose(controller);
        }
    }

    for (auto [lusIndex, controller] : sdlControllersWithGyro) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping = std::make_shared<SDLGyroMapping>(lusIndex, portIndex, 1.0f, 0.0f, 0.0f, 0.0f);
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

            mapping = std::make_shared<SDLGyroMapping>(lusIndex, portIndex, 1.0f, 0.0f, 0.0f, 0.0f);
            mapping->Recalibrate();
            break;
        }
    }

    for (auto [i, controller] : sdlControllersWithGyro) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
#endif
} // namespace LUS
