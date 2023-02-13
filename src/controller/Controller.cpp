#include "Controller.h"
#include <memory>
#include <algorithm>
#include "core/bridge/consolevariablebridge.h"
#if __APPLE__
#include <SDL_events.h>
#else
#include <SDL2/SDL_events.h>
#endif

namespace Ship {

Controller::Controller() : mIsRumbling(false) {
    mAttachment = nullptr;

    for (int32_t virtualSlot = 0; virtualSlot < MAXCONTROLLERS; virtualSlot++) {
        mProfiles[virtualSlot] = std::make_shared<DeviceProfile>();
        mButtonData[virtualSlot] = std::make_shared<Buttons>();
    }
}

int8_t Controller::ReadStick(int32_t virtualSlot, Stick stick, Axis axis) {
    switch (stick) {
        case Stick::LEFT: {
            switch (axis) {
                case Axis::X: {
                    if (getLeftStickX(virtualSlot) == 0) {
                        if (getPressedButtons(virtualSlot) & BTN_STICKLEFT) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(virtualSlot) & BTN_STICKRIGHT) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getLeftStickX(virtualSlot);
                    }
                }
                case Axis::Y: {
                    if (getLeftStickY(virtualSlot) == 0) {
                        if (getPressedButtons(virtualSlot) & BTN_STICKDOWN) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(virtualSlot) & BTN_STICKUP) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getLeftStickY(virtualSlot);
                    }
                }
            }
        }
        case Stick::RIGHT: {
            switch (axis) {
                case Axis::X: {
                    if (getRightStickX(virtualSlot) == 0) {
                        if (getPressedButtons(virtualSlot) & BTN_VSTICKLEFT) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(virtualSlot) & BTN_VSTICKRIGHT) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getRightStickX(virtualSlot);
                    }
                }
                case Axis::Y: {
                    if (getRightStickY(virtualSlot) == 0) {
                        if (getPressedButtons(virtualSlot) & BTN_VSTICKDOWN) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(virtualSlot) & BTN_VSTICKUP) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getRightStickY(virtualSlot);
                    }
                }
            }
        }
    }

    return 0;
}

void Controller::ProcessStick(int8_t& x, int8_t& y, uint16_t deadzone) {
    // create scaled circular dead-zone in range {-15 ... +15}
    auto len = sqrt(x * x + y * y);
    if (len < deadzone) {
        len = 0;
    } else if (len > MAX_AXIS_RANGE) {
        len = MAX_AXIS_RANGE / len;
    } else {
        len = (len - deadzone) * MAX_AXIS_RANGE / (MAX_AXIS_RANGE - deadzone) / len;
    }
    x *= len;
    y *= len;
}

void Controller::Read(OSContPad* pad, int32_t virtualSlot) {
    ReadFromSource(virtualSlot);

    OSContPad padToBuffer = { 0 };

#ifndef __WIIU__
    SDL_PumpEvents();
#endif

    // Button Inputs
    padToBuffer.button |= getPressedButtons(virtualSlot) & 0xFFFF;

    // Stick Inputs
    int8_t leftStickX = ReadStick(virtualSlot, LEFT, X);
    int8_t leftStickY = ReadStick(virtualSlot, LEFT, Y);
    int8_t rightStickX = ReadStick(virtualSlot, RIGHT, X);
    int8_t rightStickY = ReadStick(virtualSlot, RIGHT, Y);

    auto profile = getProfile(virtualSlot);
    ProcessStick(leftStickX, leftStickY, profile->AxisDeadzones[0]);
    ProcessStick(rightStickX, rightStickY, profile->AxisDeadzones[2]);
    
    if (pad == nullptr) {
        return;
    }
    
    padToBuffer.stick_x = leftStickX;
    padToBuffer.stick_y = leftStickY;
    padToBuffer.right_stick_x = rightStickX;
    padToBuffer.right_stick_y = rightStickY;

    // Gyro
    padToBuffer.gyro_x = getGyroX(virtualSlot);
    padToBuffer.gyro_y = getGyroY(virtualSlot);

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

void Controller::SetButtonMapping(int32_t virtualSlot, int32_t n64Button, int32_t scancode) {
    std::map<int32_t, int32_t>& mappings = getProfile(virtualSlot)->Mappings;
    std::erase_if(mappings, [n64Button](const std::pair<int32_t, int32_t>& bin) { return bin.second == n64Button; });
    mappings[scancode] = n64Button;
}

int8_t& Controller::getLeftStickX(int32_t virtualSlot) {
    return mButtonData[virtualSlot]->LeftStickX;
}

int8_t& Controller::getLeftStickY(int32_t virtualSlot) {
    return mButtonData[virtualSlot]->LeftStickY;
}

int8_t& Controller::getRightStickX(int32_t virtualSlot) {
    return mButtonData[virtualSlot]->RightStickX;
}

int8_t& Controller::getRightStickY(int32_t virtualSlot) {
    return mButtonData[virtualSlot]->RightStickY;
}

int32_t& Controller::getPressedButtons(int32_t virtualSlot) {
    return mButtonData[virtualSlot]->PressedButtons;
}

float& Controller::getGyroX(int32_t virtualSlot) {
    return mButtonData[virtualSlot]->GyroX;
}

float& Controller::getGyroY(int32_t virtualSlot) {
    return mButtonData[virtualSlot]->GyroY;
}

std::shared_ptr<DeviceProfile> Controller::getProfile(int32_t virtualSlot) {
    return mProfiles[virtualSlot];
}

std::shared_ptr<ControllerAttachment> Controller::GetAttachment() {
    return mAttachment;
}

bool Controller::IsRumbling() {
    return mIsRumbling;
}

std::string Controller::GetGuid() {
    return mGuid;
}
} // namespace Ship
