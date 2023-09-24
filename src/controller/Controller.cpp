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

#include "controller/sdl/SDLButtonToButtonMapping.h"
#include "controller/sdl/SDLAxisDirectionToButtonMapping.h"

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {

Controller::Controller() {
    mLeftStick = std::make_shared<ControllerStick>();
    mRightStick = std::make_shared<ControllerStick>();
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

void Controller::ReloadAllMappings() {
    ClearAllButtonMappings();

    AddButtonMapping(std::make_shared<SDLButtonToButtonMapping>(BTN_A, 0, 0));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CUP, 0, 3, -1));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CDOWN, 0, 3, 1));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CLEFT, 0, 2, -1));
    AddButtonMapping(std::make_shared<SDLAxisDirectionToButtonMapping>(BTN_CRIGHT, 0, 2, 1));

    GetLeftStick()->ReloadAllMappings();
    GetRightStick()->ReloadAllMappings();
}

void Controller::ReadToPad(OSContPad* pad) {
    // ReadDevice(portIndex);

    OSContPad padToBuffer = { 0 };

#ifndef __WIIU__
    SDL_PumpEvents();
#endif

    // Button Inputs
    for (auto mapping : mButtonMappings) {
        mapping->UpdatePad(padToBuffer.button);
    }

    mLeftStick->UpdatePad(padToBuffer.stick_x, padToBuffer.stick_y);
    // padToBuffer.button |= GetPressedButtons(portIndex) & 0xFFFF;

    // // Stick Inputs
    // int8_t leftStickX = ReadStick(portIndex, LEFT, X);
    // int8_t leftStickY = ReadStick(portIndex, LEFT, Y);
    // int8_t rightStickX = ReadStick(portIndex, RIGHT, X);
    // int8_t rightStickY = ReadStick(portIndex, RIGHT, Y);

    // auto profile = GetProfile(portIndex);
    // ProcessStick(leftStickX, leftStickY, profile->AxisDeadzones[0], profile->AxisDeadzones[1],
    //              profile->NotchProximityThreshold);
    // ProcessStick(rightStickX, rightStickY, profile->AxisDeadzones[2], profile->AxisDeadzones[3],
    //              profile->NotchProximityThreshold);

    // if (pad == nullptr) {
    //     return;
    // }

    // padToBuffer.stick_x = leftStickX;
    // padToBuffer.stick_y = leftStickY;
    // padToBuffer.right_stick_x = rightStickX;
    // padToBuffer.right_stick_y = rightStickY;

    // // Gyro
    // padToBuffer.gyro_x = GetGyroX(portIndex);
    // padToBuffer.gyro_y = GetGyroY(portIndex);

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
        // if (pad->gyro_x == 0) {
        //     pad->gyro_x = padFromBuffer.gyro_x;
        // }
        // if (pad->gyro_y == 0) {
        //     pad->gyro_y = padFromBuffer.gyro_y;
        // }
        // if (pad->right_stick_x == 0) {
        //     pad->right_stick_x = padFromBuffer.right_stick_x;
        // }
        // if (pad->right_stick_y == 0) {
        //     pad->right_stick_y = padFromBuffer.right_stick_y;
        // }
    }

    while (mPadBuffer.size() > 6) {
        mPadBuffer.pop_back();
    }
}

// void Controller::SetButtonMapping(int32_t portIndex, int32_t deviceButtonId, int32_t n64bitmask) {
//     GetProfile(portIndex)->Mappings[deviceButtonId] = n64bitmask;
// }

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

// std::shared_ptr<DeviceProfile> Controller::GetProfile(int32_t portIndex) {
//     return mProfiles[portIndex];
// }

// bool Controller::IsRumbling() {
//     return mIsRumbling;
// }

// Color_RGB8 Controller::GetLedColor() {
//     return mLedColor;
// }

// std::string Controller::GetGuid() {
//     return mGuid;
// }

// std::string Controller::GetControllerName() {
//     return mControllerName;
// }

// double Controller::GetClosestNotch(double angle, double approximationThreshold) {
//     constexpr auto octagonAngle = M_TAU / 8;
//     const auto closestNotch = std::round(angle / octagonAngle) * octagonAngle;
//     const auto distanceToNotch = std::abs(fmod(closestNotch - angle + M_PI, M_TAU) - M_PI);
//     return distanceToNotch < approximationThreshold / 2 ? closestNotch : angle;
// }

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
    mButtonMappings.push_back(mapping);
}

void Controller::ClearButtonMappings(uint16_t bitmask) {
    // todo: figure out why this isn't compiling
    // std::erase_if(mButtonMappings, [](std::shared_ptr<ButtonMapping> mapping, uint16_t bitmask) {
    //     return mapping->GetBitmask() == bitmask;
    // });
}

void Controller::ClearAllButtonMappings() {
    mButtonMappings.clear();
}

} // namespace LUS
