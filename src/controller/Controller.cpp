#include "Controller.h"
#include <memory>
#include <algorithm>
#include "public/bridge/consolevariablebridge.h"
#if __APPLE__
#include <SDL_events.h>
#else
#include <SDL2/SDL_events.h>
#endif
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>

#include "controller/sdl/SDLButtonToButtonMapping.h"
#include "controller/sdl/SDLAxisDirectionToButtonMapping.h"

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {

Controller::Controller(uint8_t port) : mPort(port) {
    mLeftStick = std::make_shared<ControllerStick>(LEFT_STICK);
    mRightStick = std::make_shared<ControllerStick>(RIGHT_STICK);
    mGyro = std::make_shared<ControllerGyro>();
}

Controller::~Controller() {
    SPDLOG_TRACE("destruct controller");
}

std::shared_ptr<ControllerStick> Controller::GetLeftStick() {
    return mLeftStick;
}

std::shared_ptr<ControllerStick> Controller::GetRightStick() {
    return mRightStick;
}

std::shared_ptr<ControllerGyro> Controller::GetGyro() {
    return mGyro;
}

std::unordered_map<std::string, std::shared_ptr<ButtonMapping>> Controller::GetAllButtonMappings() {
    return mButtonMappings;
}

std::shared_ptr<ButtonMapping> Controller::GetButtonMappingByUuid(std::string uuid) {
    return mButtonMappings[uuid];
}

void Controller::LoadButtonMappingFromConfig(std::string uuid) {
    // todo: maybe this stuff makes sense in a factory?
    const std::string mappingCvarKey = "gControllers.ButtonMappings." + uuid;
    const std::string mappingClass =
        CVarGetString(StringHelper::Sprintf("%s.ButtonMappingClass", mappingCvarKey.c_str()).c_str(), "");
    uint16_t bitmask = CVarGetInteger(StringHelper::Sprintf("%s.Bitmask", mappingCvarKey.c_str()).c_str(), 0);
    if (!bitmask) {
        // all button mappings need bitmasks
        CVarClear(mappingCvarKey.c_str());
        CVarSave();
        return;
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
            return;
        }

        AddButtonMapping(
            std::make_shared<SDLButtonToButtonMapping>(uuid, bitmask, sdlControllerIndex, sdlControllerButton));
        return;
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
            return;
        }

        AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(uuid, bitmask, sdlControllerIndex,
                                                                           sdlControllerAxis, axisDirection));
        return;
    }
}

uint8_t Controller::GetPort() {
    return mPort;
}

bool Controller::HasConfig() {
    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPort + 1);
    return CVarGetInteger(hasConfigCvarKey.c_str(), false);
}

void Controller::SaveButtonMappingIdsToConfig() {
    // todo: this efficently (when we build out cvar array support?)
    std::string buttonMappingIdListString = "";
    for (auto [uuid, mapping] : mButtonMappings) {
        buttonMappingIdListString += uuid;
        buttonMappingIdListString += ",";
    }

    const std::string buttonMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.ButtonMappingIds", mPort + 1);
    CVarSetString(buttonMappingIdsCvarKey.c_str(), buttonMappingIdListString.c_str());
    CVarSave();
}

void Controller::ResetToDefaultMappings(int32_t sdlControllerIndex) {
    const std::string buttonMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.ButtonMappingIds", mPort + 1);
    CVarClear(buttonMappingIdsCvarKey.c_str());

    for (auto [uuid, mapping] : mButtonMappings) {
        const std::string mappingCvarKey = "gControllers.ButtonMappings." + uuid;
        CVarClear(mappingCvarKey.c_str());
    }

    ClearAllButtonMappings();

    AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(BTN_A, sdlControllerIndex, SDL_CONTROLLER_BUTTON_A));
    AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(BTN_B, sdlControllerIndex, SDL_CONTROLLER_BUTTON_B));
    AddButtonMapping(
        std::make_shared<SDLButtonToButtonMapping>(BTN_L, sdlControllerIndex, SDL_CONTROLLER_BUTTON_LEFTSHOULDER));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_R, sdlControllerIndex,
                                                                       SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_Z, sdlControllerIndex,
                                                                       SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1));
    AddButtonMapping(
        std::make_shared<SDLButtonToButtonMapping>(BTN_START, sdlControllerIndex, SDL_CONTROLLER_BUTTON_START));
    AddButtonMapping(
        std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CUP, sdlControllerIndex, SDL_CONTROLLER_AXIS_RIGHTY, -1));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CDOWN, sdlControllerIndex,
                                                                       SDL_CONTROLLER_AXIS_RIGHTY, 1));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CLEFT, sdlControllerIndex,
                                                                       SDL_CONTROLLER_AXIS_RIGHTX, -1));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CRIGHT, sdlControllerIndex,
                                                                       SDL_CONTROLLER_AXIS_RIGHTX, 1));
    AddButtonMapping(
        std::make_shared<SDLButtonToButtonMapping>(BTN_DUP, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_UP));
    AddButtonMapping(
        std::make_shared<SDLButtonToButtonMapping>(BTN_DDOWN, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_DOWN));
    AddButtonMapping(
        std::make_shared<SDLButtonToButtonMapping>(BTN_DLEFT, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_LEFT));
    AddButtonMapping(
        std::make_shared<SDLButtonToButtonMapping>(BTN_DRIGHT, sdlControllerIndex, SDL_CONTROLLER_BUTTON_DPAD_RIGHT));

    for (auto [uuid, mapping] : mButtonMappings) {
        mapping->SaveToConfig();
    }
    SaveButtonMappingIdsToConfig();

    GetLeftStick()->ResetToDefaultMappings(mPort, sdlControllerIndex);
    GetRightStick()->ResetToDefaultMappings(mPort, sdlControllerIndex);

    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPort + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
}

void Controller::ReloadAllMappingsFromConfig() {
    ClearAllButtonMappings();

    // todo: this efficently (when we build out cvar array support?)
    // i don't expect it to really be a problem with the small number of mappings we have
    // for each controller (especially compared to include/exclude locations in rando), and
    // the audio editor pattern doesn't work for this because that looks for ids that are either
    // hardcoded or provided by an otr file
    const std::string buttonMappingIdsCvarKey =
        StringHelper::Sprintf("gControllers.Port%d.ButtonMappingIds", mPort + 1);
    std::stringstream buttonMappingIdsStringStream(CVarGetString(buttonMappingIdsCvarKey.c_str(), ""));
    std::string buttonMappingIdString;
    while (getline(buttonMappingIdsStringStream, buttonMappingIdString, ',')) {
        LoadButtonMappingFromConfig(buttonMappingIdString);
    }

    GetLeftStick()->ReloadAllMappingsFromConfig(mPort);
    GetRightStick()->ReloadAllMappingsFromConfig(mPort);
}

void Controller::ReadToPad(OSContPad* pad) {
    OSContPad padToBuffer = { 0 };

#ifndef __WIIU__
    SDL_PumpEvents();
#endif

    // Button Inputs
    for (const auto& [uuid, mapping] : mButtonMappings) {
        mapping->UpdatePad(padToBuffer.button);
    }

    // Stick Inputs
    GetLeftStick()->UpdatePad(padToBuffer.stick_x, padToBuffer.stick_y);
    GetRightStick()->UpdatePad(padToBuffer.stick_x, padToBuffer.stick_y);

    // Gyro
    GetGyro()->UpdatePad(padToBuffer.gyro_x, padToBuffer.gyro_y);

    mPadBuffer.push_front(padToBuffer);
    if (pad != nullptr) {
        auto& padFromBuffer =
            mPadBuffer[std::min(mPadBuffer.size() - 1, (size_t)CVarGetInteger("gSimulatedInputLag", 0))];
        pad->button |= padFromBuffer.button;
        if (pad->stick_x == 0) {
            pad->stick_x = padFromBuffer.stick_x;
        }
        if (pad->stick_y == 0) {
            pad->stick_y = padFromBuffer.stick_y;
        }
        if (pad->gyro_x == 0) {
            pad->gyro_x = padFromBuffer.gyro_x;
        }
        if (pad->gyro_y == 0) {
            pad->gyro_y = padFromBuffer.gyro_y;
        }
        if (pad->right_stick_x == 0) {
            pad->right_stick_x = padFromBuffer.right_stick_x;
        }
        if (pad->right_stick_y == 0) {
            pad->right_stick_y = padFromBuffer.right_stick_y;
        }
    }

    while (mPadBuffer.size() > 6) {
        mPadBuffer.pop_back();
    }
}

// int8_t& Controller::GetLeftStickX(int32_t portIndex) {
//     return mButtonData[portIndex]->LeftStickX;
// }

// int8_t& Controller::GetLeftStickY(int32_t portIndex) {
//     return mButtonData[portIndex]->LeftStickY;
// }

// int8_t& Controller::GetRightStickX(int32_t portIndex) {
//     return mButtonData[portIndex]->RightStickX;
// }

// int8_t& Controller::GetRightStickY(int32_t portIndex) {
//     return mButtonData[portIndex]->RightStickY;
// }

// int32_t& Controller::GetPressedButtons(int32_t portIndex) {
//     return mButtonData[portIndex]->PressedButtons;
// }

// float& Controller::GetGyroX(int32_t portIndex) {
//     return mButtonData[portIndex]->GyroX;
// }

// float& Controller::GetGyroY(int32_t portIndex) {
//     return mButtonData[portIndex]->GyroY;
// }

// bool Controller::IsRumbling() {
//     return mIsRumbling;
// }

// Color_RGB8 Controller::GetLedColor() {
//     return mLedColor;
// }

// void Controller::AddButtonMappingFromRawPress() {
//     SDL_GameControllerUpdate();

//     for (int32_t i = SDL_CONTROLLER_BUTTON_A; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
//         if (SDL_GameControllerGetButton(mController, static_cast<SDL_GameControllerButton>(i))) {
//             return i;
//         }
//     }

//     for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
//         const auto axis = static_cast<SDL_GameControllerAxis>(i);
//         const auto axisValue = SDL_GameControllerGetAxis(mController, axis) / 32767.0f;

//         if (axisValue < -0.7f) {
//             return -(axis + AXIS_SCANCODE_BIT);
//         }

//         if (axisValue > 0.7f) {
//             return (axis + AXIS_SCANCODE_BIT);
//         }
//     }
// }

bool Controller::AddOrEditButtonMappingFromRawPress(uint16_t bitmask, std::string uuid = "") {
    // sdl
    std::unordered_map<int32_t, SDL_GameController*> sdlControllers;
    bool result = false;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sdlControllers[i] = SDL_GameControllerOpen(i);
        }
    }

    for (auto [controllerIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                if (uuid != "") {
                    ClearButtonMapping(uuid);
                    AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(uuid, bitmask, controllerIndex, button));
                } else {
                    AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(bitmask, controllerIndex, button));
                }
                result = true;
                break;
            }
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_GameControllerClose(controller);
    }

    return result;

    // SDL_GameControllerUpdate();



    // for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
    //     const auto axis = static_cast<SDL_GameControllerAxis>(i);
    //     const auto axisValue = SDL_GameControllerGetAxis(mController, axis) / 32767.0f;

    //     if (axisValue < -0.7f) {
    //         return -(axis + AXIS_SCANCODE_BIT);
    //     }

    //     if (axisValue > 0.7f) {
    //         return (axis + AXIS_SCANCODE_BIT);
    //     }
    // }
}

bool Controller::IsConnected() const {
    return mIsConnected;
}

void Controller::Connect() {
    mIsConnected = true;
}

void Controller::Disconnect() {
    mIsConnected = false;
}

void Controller::AddButtonMapping(std::shared_ptr<ButtonMapping> mapping) {
    mButtonMappings[mapping->GetUuid()] = mapping;
}

void Controller::ClearButtonMapping(std::string uuid) {
    mButtonMappings.erase(uuid);
}

void Controller::ClearButtonMapping(std::shared_ptr<ButtonMapping> mapping) {
    ClearButtonMapping(mapping->GetUuid());
}

void Controller::ClearAllButtonMappings() {
    mButtonMappings.clear();
}

} // namespace LUS
