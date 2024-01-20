#include "ButtonMappingFactory.h"
#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>
#include "libultraship/libultra/controller.h"
#include "Context.h"
#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/wiiu/WiiUButtonToButtonMapping.h"
#include "controller/deviceindex/LUSDeviceIndexToWiiUDeviceIndexMapping.h"
#else
#include "controller/controldevice/controller/mapping/keyboard/KeyboardKeyToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLButtonToButtonMapping.h"
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#include "controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h"
#endif

namespace LUS {
std::shared_ptr<ControllerButtonMapping> ButtonMappingFactory::CreateButtonMappingFromConfig(uint8_t portIndex,
                                                                                             std::string id) {
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

#ifdef __WIIU__
    if (mappingClass == "WiiUButtonToButtonMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);
        bool isClassic = CVarGetInteger(
            StringHelper::Sprintf("%s.IsClassicControllerButton", mappingCvarKey.c_str()).c_str(), false);
        bool isNunchuk =
            CVarGetInteger(StringHelper::Sprintf("%s.IsNunchukButton", mappingCvarKey.c_str()).c_str(), false);
        int32_t wiiuControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.WiiUControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0 || wiiuControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<WiiUButtonToButtonMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex,
                                                           bitmask, isNunchuk, isClassic, wiiuControllerButton);
    }
#else
    if (mappingClass == "SDLButtonToButtonMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerButton =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(), -1);

        if (lusDeviceIndex < 0 || sdlControllerButton == -1) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLButtonToButtonMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex,
                                                          bitmask, sdlControllerButton);
    }

    if (mappingClass == "SDLAxisDirectionToButtonMapping") {
        int32_t lusDeviceIndex =
            CVarGetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(), -1);
        int32_t sdlControllerAxis =
            CVarGetInteger(StringHelper::Sprintf("%s.SDLControllerAxis", mappingCvarKey.c_str()).c_str(), -1);
        int32_t axisDirection =
            CVarGetInteger(StringHelper::Sprintf("%s.AxisDirection", mappingCvarKey.c_str()).c_str(), 0);

        if (lusDeviceIndex < 0 || sdlControllerAxis == -1 || (axisDirection != -1 && axisDirection != 1)) {
            // something about this mapping is invalid
            CVarClear(mappingCvarKey.c_str());
            CVarSave();
            return nullptr;
        }

        return std::make_shared<SDLAxisDirectionToButtonMapping>(static_cast<LUSDeviceIndex>(lusDeviceIndex), portIndex,
                                                                 bitmask, sdlControllerAxis, axisDirection);
    }

    if (mappingClass == "KeyboardKeyToButtonMapping") {
        int32_t scancode =
            CVarGetInteger(StringHelper::Sprintf("%s.KeyboardScancode", mappingCvarKey.c_str()).c_str(), 0);

        return std::make_shared<KeyboardKeyToButtonMapping>(portIndex, bitmask, static_cast<KbScancode>(scancode));
    }
#endif

    return nullptr;
}

#ifdef __WIIU__
std::vector<std::shared_ptr<ControllerButtonMapping>>
ButtonMappingFactory::CreateDefaultWiiUButtonMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                                      uint16_t bitmask) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    auto wiiuIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToWiiUDeviceIndexMapping>(
        Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(lusDeviceIndex));
    if (wiiuIndexMapping == nullptr) {
        return std::vector<std::shared_ptr<ControllerButtonMapping>>();
    }

    if (wiiuIndexMapping->IsWiiUGamepad()) {
        switch (bitmask) {
            case BTN_A:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_A, false,
                                                                               false, VPAD_BUTTON_A));
                break;
            case BTN_B:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_B, false,
                                                                               false, VPAD_BUTTON_B));
                break;
            case BTN_L:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_L, false,
                                                                               false, VPAD_BUTTON_L));
                break;
            case BTN_R:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_R, false,
                                                                               false, VPAD_BUTTON_ZR));
                break;
            case BTN_Z:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_Z, false,
                                                                               false, VPAD_BUTTON_ZL));
                break;
            case BTN_START:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_START,
                                                                               false, false, VPAD_BUTTON_PLUS));
                break;
            case BTN_CUP:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                    lusDeviceIndex, portIndex, BTN_CUP, false, false, VPAD_STICK_R_EMULATION_UP));
                break;
            case BTN_CDOWN:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                    lusDeviceIndex, portIndex, BTN_CDOWN, false, false, VPAD_STICK_R_EMULATION_DOWN));
                break;
            case BTN_CLEFT:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                    lusDeviceIndex, portIndex, BTN_CLEFT, false, false, VPAD_STICK_R_EMULATION_LEFT));
                break;
            case BTN_CRIGHT:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                    lusDeviceIndex, portIndex, BTN_CRIGHT, false, false, VPAD_STICK_R_EMULATION_RIGHT));
                break;
            case BTN_DUP:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DUP,
                                                                               false, false, VPAD_BUTTON_UP));
                break;
            case BTN_DDOWN:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DDOWN,
                                                                               false, false, VPAD_BUTTON_DOWN));
                break;
            case BTN_DLEFT:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DLEFT,
                                                                               false, false, VPAD_BUTTON_LEFT));
                break;
            case BTN_DRIGHT:
                mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DRIGHT,
                                                                               false, false, VPAD_BUTTON_RIGHT));
                break;
        }

        return mappings;
    }

    switch (wiiuIndexMapping->GetExtensionType()) {
        case WPAD_EXT_PRO_CONTROLLER:
            switch (bitmask) {
                case BTN_A:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_A,
                                                                                   false, false, WPAD_PRO_BUTTON_A));
                    break;
                case BTN_B:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_B,
                                                                                   false, false, WPAD_PRO_BUTTON_B));
                    break;
                case BTN_L:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_L,
                                                                                   false, false, WPAD_PRO_TRIGGER_L));
                    break;
                case BTN_R:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_R,
                                                                                   false, false, WPAD_PRO_TRIGGER_ZR));
                    break;
                case BTN_Z:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_Z,
                                                                                   false, false, WPAD_PRO_TRIGGER_ZL));
                    break;
                case BTN_START:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_START,
                                                                                   false, false, WPAD_PRO_BUTTON_PLUS));
                    break;
                case BTN_CUP:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CUP, false, false, WPAD_PRO_STICK_R_EMULATION_UP));
                    break;
                case BTN_CDOWN:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CDOWN, false, false, WPAD_PRO_STICK_R_EMULATION_DOWN));
                    break;
                case BTN_CLEFT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CLEFT, false, false, WPAD_PRO_STICK_R_EMULATION_LEFT));
                    break;
                case BTN_CRIGHT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CRIGHT, false, false, WPAD_PRO_STICK_R_EMULATION_RIGHT));
                    break;
                case BTN_DUP:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DUP,
                                                                                   false, false, WPAD_PRO_BUTTON_UP));
                    break;
                case BTN_DDOWN:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DDOWN,
                                                                                   false, false, WPAD_PRO_BUTTON_DOWN));
                    break;
                case BTN_DLEFT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DLEFT,
                                                                                   false, false, WPAD_PRO_BUTTON_LEFT));
                    break;
                case BTN_DRIGHT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_DRIGHT, false, false, WPAD_PRO_BUTTON_RIGHT));
                    break;
            }
            return mappings;
        case WPAD_EXT_CLASSIC:
        case WPAD_EXT_MPLUS_CLASSIC:
            switch (bitmask) {
                case BTN_A:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_A,
                                                                                   false, true, WPAD_CLASSIC_BUTTON_A));
                    break;
                case BTN_B:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_B,
                                                                                   false, true, WPAD_CLASSIC_BUTTON_B));
                    break;
                case BTN_L:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_L,
                                                                                   false, true, WPAD_CLASSIC_BUTTON_L));
                    break;
                case BTN_R:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_R, false, true, WPAD_CLASSIC_BUTTON_ZR));
                    break;
                case BTN_Z:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_Z, false, true, WPAD_CLASSIC_BUTTON_ZL));
                    break;
                case BTN_START:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_START, false, true, WPAD_CLASSIC_BUTTON_PLUS));
                    break;
                case BTN_CUP:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CUP, false, true, WPAD_CLASSIC_STICK_R_EMULATION_UP));
                    break;
                case BTN_CDOWN:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CDOWN, false, true, WPAD_CLASSIC_STICK_R_EMULATION_DOWN));
                    break;
                case BTN_CLEFT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CLEFT, false, true, WPAD_CLASSIC_STICK_R_EMULATION_LEFT));
                    break;
                case BTN_CRIGHT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CRIGHT, false, true, WPAD_CLASSIC_STICK_R_EMULATION_RIGHT));
                    break;
                case BTN_DUP:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_DUP, false, true, WPAD_CLASSIC_BUTTON_UP));
                    break;
                case BTN_DDOWN:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_DDOWN, false, true, WPAD_CLASSIC_BUTTON_DOWN));
                    break;
                case BTN_DLEFT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_DLEFT, false, true, WPAD_CLASSIC_BUTTON_LEFT));
                    break;
                case BTN_DRIGHT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_DRIGHT, false, true, WPAD_CLASSIC_BUTTON_RIGHT));
                    break;
            }
            return mappings;
        case WPAD_EXT_NUNCHUK:
        case WPAD_EXT_MPLUS_NUNCHUK:
            switch (bitmask) {
                case BTN_A:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_A,
                                                                                   false, false, WPAD_BUTTON_A));
                    break;
                case BTN_B:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_B,
                                                                                   false, false, WPAD_BUTTON_B));
                    break;
                case BTN_L:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_L,
                                                                                   true, false, WPAD_NUNCHUK_BUTTON_C));
                    break;
                case BTN_Z:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_Z, false, false, WPAD_NUNCHUK_BUTTON_Z));
                    break;
                case BTN_START:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_START,
                                                                                   false, false, WPAD_BUTTON_PLUS));
                    break;
                case BTN_CUP:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_CUP,
                                                                                   false, false, WPAD_BUTTON_UP));
                    break;
                case BTN_CDOWN:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_CDOWN,
                                                                                   false, false, WPAD_BUTTON_DOWN));
                    break;
                case BTN_CLEFT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_CLEFT,
                                                                                   false, false, WPAD_BUTTON_LEFT));
                    break;
                case BTN_CRIGHT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_CRIGHT, false, false, WPAD_BUTTON_RIGHT));
                    break;
            }
            return mappings;
        case WPAD_EXT_MPLUS:
        case WPAD_EXT_CORE:
            switch (bitmask) {
                case BTN_A:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_A,
                                                                                   false, false, WPAD_BUTTON_A));
                    break;
                case BTN_B:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_B,
                                                                                   false, false, WPAD_BUTTON_B));
                    break;
                case BTN_L:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_L,
                                                                                   false, false, WPAD_BUTTON_2));
                    break;
                case BTN_R:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_R,
                                                                                   false, false, WPAD_BUTTON_1));
                    break;
                case BTN_START:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_START,
                                                                                   false, false, WPAD_BUTTON_PLUS));
                    break;
                case BTN_DUP:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DUP,
                                                                                   false, false, WPAD_BUTTON_UP));
                    break;
                case BTN_DDOWN:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DDOWN,
                                                                                   false, false, WPAD_BUTTON_DOWN));
                    break;
                case BTN_DLEFT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DLEFT,
                                                                                   false, false, WPAD_BUTTON_LEFT));
                    break;
                case BTN_DRIGHT:
                    mappings.push_back(std::make_shared<WiiUButtonToButtonMapping>(
                        lusDeviceIndex, portIndex, BTN_DRIGHT, false, false, WPAD_BUTTON_RIGHT));
                    break;
            }
            return mappings;
        default:
            return mappings;
    }
}

std::shared_ptr<ControllerButtonMapping> ButtonMappingFactory::CreateButtonMappingFromWiiUInput(uint8_t portIndex,
                                                                                                uint16_t bitmask) {
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

                return std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, bitmask, false, false, i);
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

                return std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, bitmask, false, false, i);
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

                    return std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, bitmask, true, false,
                                                                       i);
                }
                break;
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                for (uint32_t i = WPAD_CLASSIC_BUTTON_UP; i <= WPAD_CLASSIC_STICK_R_EMULATION_UP; i <<= 1) {
                    if (!(kstatus->classic.hold & i)) {
                        continue;
                    }

                    return std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, bitmask, false, true,
                                                                       i);
                }
                break;
        }

        for (auto i : { WPAD_BUTTON_LEFT, WPAD_BUTTON_RIGHT, WPAD_BUTTON_DOWN, WPAD_BUTTON_UP, WPAD_BUTTON_PLUS,
                        WPAD_BUTTON_2, WPAD_BUTTON_1, WPAD_BUTTON_B, WPAD_BUTTON_A, WPAD_BUTTON_MINUS, WPAD_BUTTON_Z,
                        WPAD_BUTTON_C, WPAD_BUTTON_HOME }) {
            if (!(kstatus->hold & i)) {
                continue;
            }

            return std::make_shared<WiiUButtonToButtonMapping>(lusDeviceIndex, portIndex, bitmask, false, false, i);
        }
    }

    return nullptr;
}
#else
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
ButtonMappingFactory::CreateDefaultSDLButtonMappings(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex,
                                                     uint16_t bitmask) {
    std::vector<std::shared_ptr<ControllerButtonMapping>> mappings;

    auto sdlIndexMapping = std::dynamic_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(
        Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(lusDeviceIndex));
    if (sdlIndexMapping == nullptr) {
        return std::vector<std::shared_ptr<ControllerButtonMapping>>();
    }

    bool isGameCube = sdlIndexMapping->GetSDLControllerName() == "Nintendo GameCube Controller";

    switch (bitmask) {
        case BTN_A:
            mappings.push_back(
                std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_A, SDL_CONTROLLER_BUTTON_A));
            break;
        case BTN_B:
            mappings.push_back(
                std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_B, SDL_CONTROLLER_BUTTON_B));
            break;
        case BTN_L:
            if (!isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_L,
                                                                              SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
            }
            break;
        case BTN_R:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(lusDeviceIndex, portIndex, BTN_R,
                                                                                 SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1));
            break;
        case BTN_Z:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(lusDeviceIndex, portIndex, BTN_Z,
                                                                                 SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1));
            break;
        case BTN_START:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_START,
                                                                          SDL_CONTROLLER_BUTTON_START));
            break;
        case BTN_CUP:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(lusDeviceIndex, portIndex, BTN_CUP,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTY, -1));
            break;
        case BTN_CDOWN:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(lusDeviceIndex, portIndex, BTN_CDOWN,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTY, 1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_CDOWN,
                                                                              SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));
            }
            break;
        case BTN_CLEFT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(lusDeviceIndex, portIndex, BTN_CLEFT,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTX, -1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_CLEFT,
                                                                              SDL_CONTROLLER_BUTTON_Y));
            }
            break;
        case BTN_CRIGHT:
            mappings.push_back(std::make_shared<SDLAxisDirectionToButtonMapping>(lusDeviceIndex, portIndex, BTN_CRIGHT,
                                                                                 SDL_CONTROLLER_AXIS_RIGHTX, 1));
            if (isGameCube) {
                mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_CRIGHT,
                                                                              SDL_CONTROLLER_BUTTON_X));
            }
            break;
        case BTN_DUP:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DUP,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_UP));
            break;
        case BTN_DDOWN:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DDOWN,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_DOWN));
            break;
        case BTN_DLEFT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DLEFT,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_LEFT));
            break;
        case BTN_DRIGHT:
            mappings.push_back(std::make_shared<SDLButtonToButtonMapping>(lusDeviceIndex, portIndex, BTN_DRIGHT,
                                                                          SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
            break;
    }

    return mappings;
}

std::shared_ptr<ControllerButtonMapping> ButtonMappingFactory::CreateButtonMappingFromSDLInput(uint8_t portIndex,
                                                                                               uint16_t bitmask) {
    std::unordered_map<LUSDeviceIndex, SDL_GameController*> sdlControllers;
    std::shared_ptr<ControllerButtonMapping> mapping = nullptr;
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
#endif
} // namespace LUS
