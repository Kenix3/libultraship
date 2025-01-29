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
#include "utils/StringHelper.h"

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace Ship {

Controller::Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> additionalBitmasks)
    : ControlDevice(portIndex) {
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

std::unordered_map<CONTROLLERBUTTONS_T, std::shared_ptr<ControllerButton>> Controller::GetAllButtons() {
    return mButtons;
}

std::shared_ptr<ControllerButton> Controller::GetButton(CONTROLLERBUTTONS_T bitmask) {
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
    const std::string hasConfigCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.HasConfig", mPortIndex + 1);
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

void Controller::ClearAllMappingsForDeviceType(PhysicalDeviceType physicalDeviceType) {
    for (auto [bitmask, button] : GetAllButtons()) {
        button->ClearAllButtonMappingsForDeviceType(physicalDeviceType);
    }
    GetLeftStick()->ClearAllMappingsForDeviceType(physicalDeviceType);
    GetRightStick()->ClearAllMappingsForDeviceType(physicalDeviceType);

    auto gyroMapping = GetGyro()->GetGyroMapping();
    if (gyroMapping != nullptr && gyroMapping->GetPhysicalDeviceType() == physicalDeviceType) {
        GetGyro()->ClearGyroMapping();
    }

    GetRumble()->ClearAllMappingsForDeviceType(physicalDeviceType);
    GetLED()->ClearAllMappingsForDeviceType(physicalDeviceType);
}

void Controller::AddDefaultMappings(PhysicalDeviceType physicalDeviceType) {
    for (auto [bitmask, button] : GetAllButtons()) {
        button->AddDefaultMappings(physicalDeviceType);
    }
    GetLeftStick()->AddDefaultMappings(physicalDeviceType);
    GetRightStick()->AddDefaultMappings(physicalDeviceType);
    GetRumble()->AddDefaultMappings(physicalDeviceType);

    const std::string hasConfigCvarKey =
        StringHelper::Sprintf(CVAR_PREFIX_CONTROLLERS ".Port%d.HasConfig", mPortIndex + 1);
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

bool Controller::ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode) {
    bool result = false;
    for (auto [bitmask, button] : GetAllButtons()) {
        result = button->ProcessKeyboardEvent(eventType, scancode) || result;
    }
    result = GetLeftStick()->ProcessKeyboardEvent(eventType, scancode) || result;
    result = GetRightStick()->ProcessKeyboardEvent(eventType, scancode) || result;
    return result;
}

bool Controller::ProcessMouseButtonEvent(bool isPressed, MouseBtn mouseButton) {
    bool result = false;
    for (auto [bitmask, button] : GetAllButtons()) {
        result = button->ProcessMouseButtonEvent(isPressed, mouseButton) || result;
    }
    result = GetLeftStick()->ProcessMouseButtonEvent(isPressed, mouseButton) || result;
    result = GetRightStick()->ProcessMouseButtonEvent(isPressed, mouseButton) || result;
    return result;
}

bool Controller::HasMappingsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType) {
    for (auto [bitmask, button] : GetAllButtons()) {
        if (button->HasMappingsForPhysicalDeviceType(physicalDeviceType)) {
            return true;
        }
    }
    if (GetLeftStick()->HasMappingsForPhysicalDeviceType(physicalDeviceType)) {
        return true;
    }
    if (GetRightStick()->HasMappingsForPhysicalDeviceType(physicalDeviceType)) {
        return true;
    }
    if (GetGyro()->HasMappingForPhysicalDeviceType(physicalDeviceType)) {
        return true;
    }
    if (GetRumble()->HasMappingsForPhysicalDeviceType(physicalDeviceType)) {
        return true;
    }
    if (GetLED()->HasMappingsForPhysicalDeviceType(physicalDeviceType)) {
        return true;
    }

    return false;
}

std::shared_ptr<ControllerButton> Controller::GetButtonByBitmask(CONTROLLERBUTTONS_T bitmask) {
    return mButtons[bitmask];
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
} // namespace Ship

namespace LUS {
Controller::Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> additionalBitmasks)
    : Ship::Controller(portIndex, additionalBitmasks) {
}

Controller::Controller(uint8_t portIndex) : Ship::Controller(portIndex, {}) {
}

void Controller::ReadToPad(void* pad) {
    ReadToOSContPad((OSContPad*)pad);
}

void Controller::ReadToOSContPad(OSContPad* pad) {
    OSContPad padToBuffer = { 0 };

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
            mPadBuffer[std::min(mPadBuffer.size() - 1, (size_t)CVarGetInteger(CVAR_SIMULATED_INPUT_LAG, 0))];

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
} // namespace LUS
