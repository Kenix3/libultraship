#include "RumbleMappingFactory.h"
#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/wiiu/WiiURumbleMapping.h"
#else
#include "controller/controldevice/controller/mapping/sdl/SDLRumbleMapping.h"
#endif
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h"

namespace LUS {
std::shared_ptr<ControllerRumbleMapping> RumbleMappingFactory::CreateRumbleMappingFromConfig(uint8_t portIndex,
                                                                                             std::string id) {
    const std::string mappingCvarKey = "gControllers.RumbleMappings." + id;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.RumbleMappingClass", mappingCvarKey.c_str()).c_str(), "");

    int32_t lowFrequencyIntensityPercentage =
        CVarGetInteger(StringHelper::Sprintf("%s.LowFrequencyIntensity", mappingCvarKey.c_str()).c_str(), -1);
    int32_t highFrequencyIntensityPercentage =
        CVarGetInteger(StringHelper::Sprintf("%s.HighFrequencyIntensity", mappingCvarKey.c_str()).c_str(), -1);

    if (lowFrequencyIntensityPercentage < 0 || lowFrequencyIntensityPercentage > 100 ||
        highFrequencyIntensityPercentage < 0 || highFrequencyIntensityPercentage > 100) {
        // something about this mapping is invalid
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return nullptr;
    }

#ifdef __WIIU__
    if (mappingClass == "WiiURumbleMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<WiiURumbleMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex,
                                                   lowFrequencyIntensityPercentage, highFrequencyIntensityPercentage);
    }
#else
    if (mappingClass == "SDLRumbleMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLRumbleMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex,
                                                  lowFrequencyIntensityPercentage, highFrequencyIntensityPercentage);
    }
#endif

    return nullptr;
}

#ifdef __WIIU__
std::vector<std::shared_ptr<ControllerRumbleMapping>>
RumbleMappingFactory::CreateDefaultWiiURumbleMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex) {
    auto wiiuIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToWiiUDeviceIndexMapping>(
        Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(lusDeviceIndex));
    if (wiiuIndexMapping == nullptr) {
        return std::vector<std::shared_ptr<ControllerRumbleMapping>>();
    }

    std::vector<std::shared_ptr<ControllerRumbleMapping>> mappings = { std::make_shared<WiiURumbleMapping>(
        lusDeviceIndex, portIndex, DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE, DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE) };

    return mappings;
}

std::shared_ptr<ControllerRumbleMapping> RumbleMappingFactory::CreateRumbleMappingFromWiiUInput(uint8_t portIndex) {
    for (auto [lusDeviceIndex, indexMapping] :
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

                return std::make_shared<WiiURumbleMapping>(lusDeviceIndex, portIndex,
                                                           DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                                                           DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
            }

            continue;
        }

        KPADError kerror;
        KPADStatus* kstatus =
            LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(wiiuIndexMapping->GetDeviceChannel()), &kerror);

        if (kstatus == nullptr || kerror != KPAD_ERROR_OK) {
            continue;
        }

        if (wiiuIndexMapping->GetExtensionType() == WPAD_EXT_PRO_CONTROLLER) {
            for (uint32_t i = WPAD_PRO_BUTTON_UP; i <= WPAD_PRO_STICK_R_EMULATION_UP; i <<= 1) {
                if (!(kstatus->pro.hold & i)) {
                    continue;
                }

                return std::make_shared<WiiURumbleMapping>(lusDeviceIndex, portIndex,
                                                           DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                                                           DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
            }

            continue;
        }

        switch (wiiuIndexMapping->GetExtensionType()) {
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
                for (auto i : { WPAD_NUNCHUK_STICK_EMULATION_LEFT, WPAD_NUNCHUK_STICK_EMULATION_RIGHT,
                                WPAD_NUNCHUK_STICK_EMULATION_DOWN, WPAD_NUNCHUK_STICK_EMULATION_UP,
                                WPAD_NUNCHUK_BUTTON_Z, WPAD_NUNCHUK_BUTTON_C }) {
                    if (!(kstatus->nunchuck.hold & i)) {
                        continue;
                    }

                    return std::make_shared<WiiURumbleMapping>(lusDeviceIndex, portIndex,
                                                               DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                                                               DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
                }
                break;
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                for (uint32_t i = WPAD_CLASSIC_BUTTON_UP; i <= WPAD_CLASSIC_STICK_R_EMULATION_UP; i <<= 1) {
                    if (!(kstatus->classic.hold & i)) {
                        continue;
                    }

                    return std::make_shared<WiiURumbleMapping>(lusDeviceIndex, portIndex,
                                                               DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                                                               DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
                }
                break;
        }

        for (auto i : { WPAD_BUTTON_LEFT, WPAD_BUTTON_RIGHT, WPAD_BUTTON_DOWN, WPAD_BUTTON_UP, WPAD_BUTTON_PLUS,
                        WPAD_BUTTON_2, WPAD_BUTTON_1, WPAD_BUTTON_B, WPAD_BUTTON_A, WPAD_BUTTON_MINUS, WPAD_BUTTON_Z,
                        WPAD_BUTTON_C, WPAD_BUTTON_HOME }) {
            if (!(kstatus->hold & i)) {
                continue;
            }

            return std::make_shared<WiiURumbleMapping>(lusDeviceIndex, portIndex,
                                                       DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                                                       DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
        }
    }

    return nullptr;
}
#else
std::vector<std::shared_ptr<ControllerRumbleMapping>>
RumbleMappingFactory::CreateDefaultSDLRumbleMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex) {
    auto sdlIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(
        Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(lusDeviceIndex));
    if (sdlIndexMapping == nullptr) {
        return std::vector<std::shared_ptr<ControllerRumbleMapping>>();
    }

    std::vector<std::shared_ptr<ControllerRumbleMapping>> mappings = { std::make_shared<SDLRumbleMapping>(
        lusDeviceIndex, portIndex, DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE, DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE) };

    return mappings;
}

std::shared_ptr<ControllerRumbleMapping> RumbleMappingFactory::CreateRumbleMappingFromSDLInput(uint8_t portIndex) {
    std::unordered_map<LUSDeviceIndex, SDL_GameController*> sdlControllersWithRumble;
    std::shared_ptr<ControllerRumbleMapping> mapping = nullptr;
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
#ifdef __SWITCH__
        bool hasRumble = false;
#else
        bool hasRumble = SDL_GameControllerHasRumble(controller);
#endif
        if (hasRumble) {
            sdlControllersWithRumble[lusIndex] = SDL_GameControllerOpen(sdlIndex);
        } else {
            SDL_GameControllerClose(controller);
        }
    }

    for (auto [lusIndex, controller] : sdlControllersWithRumble) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                mapping =
                    std::make_shared<SDLRumbleMapping>(lusIndex, portIndex, DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                                                       DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
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

            mapping = std::make_shared<SDLRumbleMapping>(lusIndex, portIndex, DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                                                         DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE);
            break;
        }
    }

    for (auto [i, controller] : sdlControllersWithRumble) {
        SDL_GameControllerClose(controller);
    }

    return mapping;
}
#endif
} // namespace LUS
