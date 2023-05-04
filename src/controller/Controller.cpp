#include "Controller.h"
#include <memory>
#include <algorithm>
#include "core/bridge/consolevariablebridge.h"
#if __APPLE__
#include <SDL_events.h>
#else
#include <SDL2/SDL_events.h>
#endif
#include <spdlog/spdlog.h>

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace Ship {

Controller::Controller(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex)
    : mIsRumbling(false), mAttachment(nullptr), mDeviceIndex(deviceIndex), mControlDeck(controlDeck) {
    for (int32_t portIndex = 0; portIndex < MAXCONTROLLERS; portIndex++) {
        mProfiles[portIndex] = std::make_shared<DeviceProfile>();
        mButtonData[portIndex] = std::make_shared<Buttons>();
    }
}

int8_t Controller::ReadStick(int32_t portIndex, Stick stick, Axis axis) {
    switch (stick) {
        case Stick::LEFT: {
            switch (axis) {
                case Axis::X: {
                    if (getLeftStickX(portIndex) == 0) {
                        if (getPressedButtons(portIndex) & BTN_STICKLEFT) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(portIndex) & BTN_STICKRIGHT) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getLeftStickX(portIndex);
                    }
                    break;
                }
                case Axis::Y: {
                    if (getLeftStickY(portIndex) == 0) {
                        if (getPressedButtons(portIndex) & BTN_STICKDOWN) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(portIndex) & BTN_STICKUP) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getLeftStickY(portIndex);
                    }
                    break;
                }
            }
            break;
        }
        case Stick::RIGHT: {
            switch (axis) {
                case Axis::X: {
                    if (getRightStickX(portIndex) == 0) {
                        if (getPressedButtons(portIndex) & BTN_VSTICKLEFT) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(portIndex) & BTN_VSTICKRIGHT) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getRightStickX(portIndex);
                    }
                    break;
                }
                case Axis::Y: {
                    if (getRightStickY(portIndex) == 0) {
                        if (getPressedButtons(portIndex) & BTN_VSTICKDOWN) {
                            return -MAX_AXIS_RANGE;
                        } else if (getPressedButtons(portIndex) & BTN_VSTICKUP) {
                            return MAX_AXIS_RANGE;
                        }
                    } else {
                        return getRightStickY(portIndex);
                    }
                    break;
                }
            }
            break;
        }
    }

    return 0;
}

void Controller::ProcessStick(int8_t& x, int8_t& y, float deadzoneX, float deadzoneY, int32_t notchProxmityThreshold) {
    auto ux = fabs(x);
    auto uy = fabs(y);

    // TODO: handle deadzones separately for X and Y
    if (deadzoneX != deadzoneY) {
        SPDLOG_TRACE("Invalid Deadzone configured. Up/Down was {} and Left/Right is {}", deadzoneY, deadzoneX);
    }

    // create scaled circular dead-zone in range {-15 ... +15}
    auto len = sqrt(ux * ux + uy * uy);
    if (len < deadzoneX) {
        len = 0;
    } else if (len > MAX_AXIS_RANGE) {
        len = MAX_AXIS_RANGE / len;
    } else {
        len = (len - deadzoneX) * MAX_AXIS_RANGE / (MAX_AXIS_RANGE - deadzoneX) / len;
    }
    ux *= len;
    uy *= len;

    // bound diagonals to an octagonal range {-68 ... +68}
    if (ux != 0.0 && uy != 0.0) {
        auto slope = uy / ux;
        auto edgex = copysign(MAX_AXIS_RANGE / (fabs(slope) + 16.0 / 69.0), ux);
        auto edgey = copysign(std::min(fabs(edgex * slope), MAX_AXIS_RANGE / (1.0 / fabs(slope) + 16.0 / 69.0)), y);
        edgex = edgey / slope;

        auto scale = sqrt(edgex * edgex + edgey * edgey) / MAX_AXIS_RANGE;
        ux *= scale;
        uy *= scale;
    }

    // map to virtual notches
    const double notchProximityValRadians = notchProxmityThreshold * M_TAU / 360;

    const double distance = std::sqrt((ux * ux) + (uy * uy)) / MAX_AXIS_RANGE;
    if (distance >= MINIMUM_RADIUS_TO_MAP_NOTCH) {
        auto angle = atan2(uy, ux) + M_TAU;
        auto newAngle = GetClosestNotch(angle, notchProximityValRadians);

        ux = cos(newAngle) * distance * MAX_AXIS_RANGE;
        uy = sin(newAngle) * distance * MAX_AXIS_RANGE;
    }

    // assign back to original sign
    x = copysign(ux, x);
    y = copysign(uy, y);
}

void Controller::ReadToPad(OSContPad* pad, int32_t portIndex) {
    ReadDevice(portIndex);

    OSContPad padToBuffer = { 0 };

#ifndef __WIIU__
    SDL_PumpEvents();
#endif

    // Button Inputs
    padToBuffer.button |= getPressedButtons(portIndex) & 0xFFFF;

    // Stick Inputs
    int8_t leftStickX = ReadStick(portIndex, LEFT, X);
    int8_t leftStickY = ReadStick(portIndex, LEFT, Y);
    int8_t rightStickX = ReadStick(portIndex, RIGHT, X);
    int8_t rightStickY = ReadStick(portIndex, RIGHT, Y);

    auto profile = getProfile(portIndex);
    ProcessStick(leftStickX, leftStickY, profile->AxisDeadzones[0], profile->AxisDeadzones[1],
                 profile->NotchProximityThreshold);
    ProcessStick(rightStickX, rightStickY, profile->AxisDeadzones[2], profile->AxisDeadzones[3],
                 profile->NotchProximityThreshold);

    if (pad == nullptr) {
        return;
    }

    padToBuffer.stick_x = leftStickX;
    padToBuffer.stick_y = leftStickY;
    padToBuffer.right_stick_x = rightStickX;
    padToBuffer.right_stick_y = rightStickY;

    // Gyro
    padToBuffer.gyro_x = getGyroX(portIndex);
    padToBuffer.gyro_y = getGyroY(portIndex);

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

void Controller::SetButtonMapping(int32_t portIndex, int32_t n64Button, int32_t scancode) {
    std::map<int32_t, int32_t>& mappings = getProfile(portIndex)->Mappings;
    std::erase_if(mappings, [n64Button](const std::pair<int32_t, int32_t>& bin) { return bin.second == n64Button; });
    mappings[scancode] = n64Button;
}

int8_t& Controller::getLeftStickX(int32_t portIndex) {
    return mButtonData[portIndex]->LeftStickX;
}

int8_t& Controller::getLeftStickY(int32_t portIndex) {
    return mButtonData[portIndex]->LeftStickY;
}

int8_t& Controller::getRightStickX(int32_t portIndex) {
    return mButtonData[portIndex]->RightStickX;
}

int8_t& Controller::getRightStickY(int32_t portIndex) {
    return mButtonData[portIndex]->RightStickY;
}

int32_t& Controller::getPressedButtons(int32_t portIndex) {
    return mButtonData[portIndex]->PressedButtons;
}

float& Controller::getGyroX(int32_t portIndex) {
    return mButtonData[portIndex]->GyroX;
}

float& Controller::getGyroY(int32_t portIndex) {
    return mButtonData[portIndex]->GyroY;
}

std::shared_ptr<DeviceProfile> Controller::getProfile(int32_t portIndex) {
    return mProfiles[portIndex];
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

std::string Controller::GetControllerName() {
    return mControllerName;
}

double Controller::GetClosestNotch(double angle, double approximationThreshold) {
    constexpr auto octagonAngle = M_TAU / 8;
    const auto closestNotch = std::round(angle / octagonAngle) * octagonAngle;
    const auto distanceToNotch = std::abs(fmod(closestNotch - angle + M_PI, M_TAU) - M_PI);
    return distanceToNotch < approximationThreshold / 2 ? closestNotch : angle;
}

std::shared_ptr<ControlDeck> Controller::GetControlDeck() {
    return mControlDeck;
}
} // namespace Ship
