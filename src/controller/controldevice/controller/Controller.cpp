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

Controller::Controller(uint8_t portIndex, std::vector<uint16_t> additionalBitmasks) : ControlDevice(portIndex) {
    for (auto bitmask : { BUTTON_BITMASKS }) {
        mButtons[bitmask] = std::make_shared<ControllerButton>(portIndex, bitmask);
    }
    for (auto bitmask : additionalBitmasks) {
        mButtons[bitmask] = std::make_shared<ControllerButton>(portIndex, bitmask);
    }
    mLeftStick = std::make_shared<ControllerStick>(portIndex, LEFT_STICK);
    mRightStick = std::make_shared<ControllerStick>(portIndex, RIGHT_STICK);
    mGyro = std::make_shared<ControllerGyro>(portIndex);
    mRumble = std::make_shared<ControllerRumble>(portIndex);
    mLED = std::make_shared<ControllerLED>(portIndex);
}

Controller::Controller(uint8_t portIndex) : Controller(portIndex, {}) {
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

void Controller::ClearAllMappingsForDevice(LUSDeviceIndex lusDeviceIndex) {
    for (auto [bitmask, button] : GetAllButtons()) {
        button->ClearAllButtonMappingsForDevice(lusDeviceIndex);
    }
    GetLeftStick()->ClearAllMappingsForDevice(lusDeviceIndex);
    GetRightStick()->ClearAllMappingsForDevice(lusDeviceIndex);

    auto gyroMapping = GetGyro()->GetGyroMapping();
    if (gyroMapping != nullptr && gyroMapping->GetLUSDeviceIndex() == lusDeviceIndex) {
        GetGyro()->ClearGyroMapping();
    }

    GetRumble()->ClearAllMappingsForDevice(lusDeviceIndex);
    GetLED()->ClearAllMappingsForDevice(lusDeviceIndex);
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

    // Button Inputs
    int32_t blarg = 3;
    for (auto [bitmask, button] : mButtons) {
        button->UpdatePad(padToBuffer.button);
        blarg++;
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

#ifndef __WIIU__
bool Controller::ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode) {
    bool result = false;
    for (auto [bitmask, button] : GetAllButtons()) {
        result = button->ProcessKeyboardEvent(eventType, scancode) || result;
    }
    result = GetLeftStick()->ProcessKeyboardEvent(eventType, scancode) || result;
    result = GetRightStick()->ProcessKeyboardEvent(eventType, scancode) || result;
    return result;
}
#endif

bool Controller::HasMappingsForLUSDeviceIndex(LUSDeviceIndex lusIndex) {
    for (auto [bitmask, button] : GetAllButtons()) {
        if (button->HasMappingsForLUSDeviceIndex(lusIndex)) {
            return true;
        }
    }
    if (GetLeftStick()->HasMappingsForLUSDeviceIndex(lusIndex)) {
        return true;
    }
    if (GetRightStick()->HasMappingsForLUSDeviceIndex(lusIndex)) {
        return true;
    }
    if (GetGyro()->HasMappingForLUSDeviceIndex(lusIndex)) {
        return true;
    }
    if (GetRumble()->HasMappingsForLUSDeviceIndex(lusIndex)) {
        return true;
    }
    if (GetLED()->HasMappingsForLUSDeviceIndex(lusIndex)) {
        return true;
    }

    return false;
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

    for (auto stick : { GetLeftStick(), GetRightStick() }) {
        auto newControllerStick =
            stick->LeftOrRightStick() == LEFT_STICK ? newController->GetLeftStick() : newController->GetRightStick();
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

    if (GetGyro()->GetGyroMapping() != nullptr && GetGyro()->GetGyroMapping()->GetLUSDeviceIndex() == lusIndex) {
        GetGyro()->GetGyroMapping()->SetPortIndex(newController->GetPortIndex());
        GetGyro()->GetGyroMapping()->SaveToConfig();

        auto oldGyroMappingFromNewController = newController->GetGyro()->GetGyroMapping();
        if (oldGyroMappingFromNewController != nullptr) {
            oldGyroMappingFromNewController->SetPortIndex(GetPortIndex());
            oldGyroMappingFromNewController->SaveToConfig();
        }
        newController->GetGyro()->SetGyroMapping(GetGyro()->GetGyroMapping());
        newController->GetGyro()->SaveGyroMappingIdToConfig();
        GetGyro()->SetGyroMapping(oldGyroMappingFromNewController);
        GetGyro()->SaveGyroMappingIdToConfig();
    }

    std::vector<std::string> rumbleMappingIdsToRemove;
    for (auto [id, mapping] : GetRumble()->GetAllRumbleMappings()) {
        if (mapping->GetLUSDeviceIndex() == lusIndex) {
            rumbleMappingIdsToRemove.push_back(id);

            mapping->SetPortIndex(newController->GetPortIndex());
            mapping->SaveToConfig();

            newController->GetRumble()->AddRumbleMapping(mapping);
        }
    }
    newController->GetRumble()->SaveRumbleMappingIdsToConfig();
    for (auto id : rumbleMappingIdsToRemove) {
        GetRumble()->ClearRumbleMappingId(id);
    }

    std::vector<std::string> ledMappingIdsToRemove;
    for (auto [id, mapping] : GetLED()->GetAllLEDMappings()) {
        if (mapping->GetLUSDeviceIndex() == lusIndex) {
            ledMappingIdsToRemove.push_back(id);

            mapping->SetPortIndex(newController->GetPortIndex());
            mapping->SaveToConfig();

            newController->GetLED()->AddLEDMapping(mapping);
        }
    }
    newController->GetLED()->SaveLEDMappingIdsToConfig();
    for (auto id : ledMappingIdsToRemove) {
        GetLED()->ClearLEDMappingId(id);
    }
}

std::vector<std::shared_ptr<ControllerMapping>> Controller::GetAllMappings() {
    std::vector<std::shared_ptr<ControllerMapping>> allMappings;
    for (auto [bitmask, button] : GetAllButtons()) {
        for (auto [id, mapping] : button->GetAllButtonMappings()) {
            allMappings.push_back(mapping);
        }
    }

    for (auto stick : { GetLeftStick(), GetRightStick() }) {
        for (auto [direction, mappings] : stick->GetAllAxisDirectionMappings()) {
            for (auto [id, mapping] : mappings) {
                allMappings.push_back(mapping);
            }
        }
    }

    allMappings.push_back(GetGyro()->GetGyroMapping());

    for (auto [id, mapping] : GetRumble()->GetAllRumbleMappings()) {
        allMappings.push_back(mapping);
    }

    for (auto [id, mapping] : GetLED()->GetAllLEDMappings()) {
        allMappings.push_back(mapping);
    }

    return allMappings;
}
} // namespace LUS
