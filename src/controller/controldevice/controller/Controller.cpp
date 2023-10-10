#include "controller/controldevice/controller/Controller.h"
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

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {

Controller::Controller(uint8_t portIndex) : ControlDevice(portIndex) {
    for (auto bitmask : { BUTTON_BITMASKS }) {
        mButtons[bitmask] = std::make_shared<ControllerButton>(portIndex, bitmask);
    }
    mLeftStick = std::make_shared<ControllerStick>(portIndex, LEFT_STICK);
    mRightStick = std::make_shared<ControllerStick>(portIndex, RIGHT_STICK);
    mGyro = std::make_shared<ControllerGyro>(portIndex);
    mRumble = std::make_shared<ControllerRumble>(portIndex);
    mLED = std::make_shared<ControllerLED>(portIndex);
}

Controller::~Controller() {
    SPDLOG_TRACE("destruct controller");
}

std::unordered_map<uint16_t, std::shared_ptr<ControllerButton>> Controller::GetAllButtons() {
    return mButtons;
}

std::shared_ptr<ControllerButton> Controller::GetButton(uint16_t bitmask) {
    return mButtons[bitmask];
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

std::shared_ptr<ControllerRumble> Controller::GetRumble() {
    return mRumble;
}

std::shared_ptr<ControllerLED> Controller::GetLED() {
    return mLED;
}

uint8_t Controller::GetPortIndex() {
    return mPortIndex;
}

bool Controller::HasConfig() {
    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPortIndex + 1);
    return CVarGetInteger(hasConfigCvarKey.c_str(), false);
}

void Controller::ClearAllMappings() {
    for (auto [bitmask, button] : GetAllButtons()) {
        button->ClearAllButtonMappings();
    }
    GetLeftStick()->ClearAllMappings();
    GetRightStick()->ClearAllMappings();
    GetGyro()->ClearGyroMapping();
    GetRumble()->ClearAllMappings();
    GetLED()->ClearAllMappings();
}

void Controller::AddDefaultMappings(LUSDeviceIndex lusDeviceIndex) {
    for (auto [bitmask, button] : GetAllButtons()) {
        button->AddDefaultMappings(lusDeviceIndex);
    }
    GetLeftStick()->AddDefaultMappings(lusDeviceIndex);
    GetRumble()->AddDefaultMappings(lusDeviceIndex);

    const std::string hasConfigCvarKey = StringHelper::Sprintf("gControllers.Port%d.HasConfig", mPortIndex + 1);
    CVarSetInteger(hasConfigCvarKey.c_str(), true);
    CVarSave();
}

void Controller::ReloadAllMappingsFromConfig() {
    for (auto [bitmask, button] : GetAllButtons()) {
        button->ReloadAllMappingsFromConfig();
    }
    GetLeftStick()->ReloadAllMappingsFromConfig();
    GetRightStick()->ReloadAllMappingsFromConfig();
    GetGyro()->ReloadGyroMappingFromConfig();
    GetRumble()->ReloadAllMappingsFromConfig();
    GetLED()->ReloadAllMappingsFromConfig();
}

void Controller::ReadToPad(OSContPad* pad) {
    OSContPad padToBuffer = { 0 };

#ifndef __WIIU__
    SDL_PumpEvents();
#endif

    // Button Inputs
    for (auto [bitmask, button] : mButtons) {
        button->UpdatePad(padToBuffer.button);
    }

    // Stick Inputs
    GetLeftStick()->UpdatePad(padToBuffer.stick_x, padToBuffer.stick_y);
    GetRightStick()->UpdatePad(padToBuffer.right_stick_x, padToBuffer.right_stick_y);

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

        if (pad->right_stick_x == 0) {
            pad->right_stick_x = padFromBuffer.right_stick_x;
        }
        if (pad->right_stick_y == 0) {
            pad->right_stick_y = padFromBuffer.right_stick_y;
        }

        if (pad->gyro_x == 0) {
            pad->gyro_x = padFromBuffer.gyro_x;
        }
        if (pad->gyro_y == 0) {
            pad->gyro_y = padFromBuffer.gyro_y;
        }
    }

    while (mPadBuffer.size() > 6) {
        mPadBuffer.pop_back();
    }
}

bool Controller::ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode) {
    bool result = false;
    for (auto [bitmask, button] : GetAllButtons()) {
        result = result || button->ProcessKeyboardEvent(eventType, scancode);
    }
    result = result || GetLeftStick()->ProcessKeyboardEvent(eventType, scancode);
    result = result || GetRightStick()->ProcessKeyboardEvent(eventType, scancode);
    return result;
}

bool Controller::HasMappingsForLUSDeviceIndex(LUSDeviceIndex lusIndex) {
    return std::any_of(GetAllButtons().begin(), GetAllButtons().end(),
                        [lusIndex](const auto& button) {
                            return button.second->HasMappingsForLUSDeviceIndex(lusIndex);
                        }) || 
        GetLeftStick()->HasMappingsForLUSDeviceIndex(lusIndex) || 
        GetRightStick()->HasMappingsForLUSDeviceIndex(lusIndex) || 
        GetGyro()->HasMappingForLUSDeviceIndex(lusIndex) || 
        GetRumble()->HasMappingsForLUSDeviceIndex(lusIndex) || 
        GetLED()->HasMappingsForLUSDeviceIndex(lusIndex);
}

std::shared_ptr<ControllerButton> Controller::GetButtonByBitmask(uint16_t bitmask) {
    return mButtons[bitmask];
}

void Controller::MoveMappingsToDifferentController(std::shared_ptr<Controller> newController, LUSDeviceIndex lusIndex) {
    for (auto [bitmask, button] : GetAllButtons()) {
        std::vector<std::string> buttonMappingIdsToRemove;
        for (auto [id, mapping] : button->GetAllButtonMappings()) {
            if (mapping->GetLUSDeviceIndex() == lusIndex) { 
                buttonMappingIdsToRemove.push_back(id);
                
                mapping->SetPortIndex(newController->GetPortIndex());
                mapping->SaveToConfig();
                
                newController->GetButtonByBitmask(bitmask)->AddButtonMapping(mapping);
            }
        }
        newController->GetButtonByBitmask(bitmask)->SaveButtonMappingIdsToConfig();
        for (auto id : buttonMappingIdsToRemove) {
            button->ClearButtonMappingId(id);
        }
    }

    for (auto stick : {GetLeftStick(), GetRightStick()}) {
        auto newControllerStick = stick->LeftOrRightStick() == LEFT_STICK ? newController->GetLeftStick() : newController->GetRightStick();
        for (auto [direction, mappings] : stick->GetAllAxisDirectionMappings()) {
            std::vector<std::string> axisDirectionMappingIdsToRemove;
            for (auto [id, mapping] : mappings) {
                if (mapping->GetLUSDeviceIndex() == lusIndex) {
                    axisDirectionMappingIdsToRemove.push_back(id);

                    mapping->SetPortIndex(newController->GetPortIndex());
                    mapping->SaveToConfig();
                    
                    newControllerStick->AddAxisDirectionMapping(direction, mapping);
                }
            }
            newControllerStick->SaveAxisDirectionMappingIdsToConfig();
            for (auto id : axisDirectionMappingIdsToRemove) {
                stick->ClearAxisDirectionMappingId(direction, id);
            }
        }
    }

    // todo: gyro after implementing multi-gyro
    // todo: handle multiple gyro mappings because this will clear the one we're moving to
    // if (GetGyro()->GetGyroMapping()->GetLUSDeviceIndex() == lusIndex) {
    //     GetGyro()->GetGyroMapping()->SetPortIndex(newController->GetPortIndex());
    //     GetGyro()->GetGyroMapping()->SaveToConfig();
    //     newController->GetGyro()->SetGyroMapping()

    // }
}
} // namespace LUS
