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

// HEY HEY HEY REMOVE THIS
#include "controller/controldevice/controller/mapping/sdl/SDLGyroMapping.h"
// END OF HEY HEY HEY REMOVE THIS

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {

Controller::Controller(uint8_t port) : ControlDevice(port) {
    for (auto bitmask : { BUTTON_BITMASKS }) {
        mButtons[bitmask] = std::make_shared<ControllerButton>(port, bitmask);
    }
    mLeftStick = std::make_shared<ControllerStick>(port, LEFT_STICK);
    mRightStick = std::make_shared<ControllerStick>(port, RIGHT_STICK);
    mGyro = std::make_shared<ControllerGyro>(port);
    mRumble = std::make_shared<ControllerRumble>(port);
    mLED = std::make_shared<ControllerLED>(port);
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

void Controller::ResetToDefaultMappings(bool keyboard, bool sdl, int32_t sdlControllerIndex) {
    for (auto [bitmask, button] : GetAllButtons()) {
        button->ResetToDefaultMappings(keyboard, sdl, sdlControllerIndex);
    }
    GetLeftStick()->ResetToDefaultMappings(keyboard, sdl, sdlControllerIndex);
    GetRightStick()->ClearAllMappings();
    GetGyro()->ClearGyroMapping();
    // HEYHEYHEY REMOVE
    GetGyro()->SetGyroMapping(std::make_shared<SDLGyroMapping>(0, 1.0f, 0.0f, 0.0f, 0.0f, 0));
    // END OF HEYHEYHEY REMOVE
    GetRumble()->ResetToDefaultMappings(sdl, sdlControllerIndex);
    GetLED()->ClearAllMappings();

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
} // namespace LUS
