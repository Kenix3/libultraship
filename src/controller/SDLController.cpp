#define NOMINMAX

#include "SDLController.h"

#include <spdlog/spdlog.h>
#include "core/Window.h"
#include <Utils/StringHelper.h>

#ifdef _MSC_VER
#define strdup _strdup
#endif

extern "C" uint8_t __osMaxControllers;

namespace Ship {

SDLController::SDLController(int32_t physicalSlot) : Controller(), mController(nullptr), mPhysicalSlot(physicalSlot) {
}

bool SDLController::Open() {
    const auto newCont = SDL_GameControllerOpen(mPhysicalSlot);

    // We failed to load the controller. Go to next.
    if (newCont == nullptr) {
        SPDLOG_ERROR("SDL Controller failed to open: ({})", SDL_GetError());
        return false;
    }

    mSupportsGyro = false;
    if (SDL_GameControllerHasSensor(newCont, SDL_SENSOR_GYRO)) {
        SDL_GameControllerSetSensorEnabled(newCont, SDL_SENSOR_GYRO, SDL_TRUE);
        mSupportsGyro = true;
    }

    char GuidBuf[33];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(mPhysicalSlot), GuidBuf, sizeof(GuidBuf));
    mController = newCont;

#ifdef __SWITCH__
    mGuid = StringHelper::Sprintf("%s:%d", GuidBuf, mPhysicalSlot);
    mControllerName = StringHelper::Sprintf("%s #%d", SDL_GameControllerNameForIndex(mPhysicalSlot), mPhysicalSlot + 1);
#else
    mGuid = std::string(GuidBuf);
    mControllerName = std::string(SDL_GameControllerNameForIndex(mPhysicalSlot));
#endif
    return true;
}

bool SDLController::Close() {
    if (mController != nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        if (CanRumble()) {
            SDL_GameControllerRumble(mController, 0, 0, 0);
        }
        SDL_GameControllerClose(mController);
    }
    mController = nullptr;

    return true;
}

void SDLController::NormalizeStickAxis(SDL_GameControllerAxis axisX, SDL_GameControllerAxis axisY,
                                       int16_t axisThreshold, int32_t virtualSlot) {
    auto profile = getProfile(virtualSlot);

    const auto axisValueX = SDL_GameControllerGetAxis(mController, axisX);
    const auto axisValueY = SDL_GameControllerGetAxis(mController, axisY);

    // scale {-32768 ... +32767} to {-84 ... +84}
    auto ax = axisValueX * 85.0f / 32767.0f;
    auto ay = axisValueY * 85.0f / 32767.0f;

    // create scaled circular dead-zone in range {-15 ... +15}
    auto len = sqrt(ax * ax + ay * ay);
    if (len < axisThreshold) {
        len = 0.0f;
    } else if (len > 85.0) {
        len = 85.0f / len;
    } else {
        len = (len - axisThreshold) * 85.0f / (85.0f - axisThreshold) / len;
    }
    ax *= len;
    ay *= len;

    // bound diagonals to an octagonal range {-68 ... +68}
    if (ax != 0.0f && ay != 0.0f) {
        auto slope = ay / ax;
        auto edgex = copysign(85.0f / (abs(slope) + 16.0f / 69.0f), ax);
        auto edgey = copysign(std::min(abs(edgex * slope), 85.0f / (1.0f / abs(slope) + 16.0f / 69.0f)), ay);
        edgex = edgey / slope;

        auto scale = sqrt(edgex * edgex + edgey * edgey) / 85.0f;
        ax *= scale;
        ay *= scale;
    }

    if (axisX == SDL_CONTROLLER_AXIS_LEFTX) {
        getLeftStickX(virtualSlot) = +ax;
        getLeftStickY(virtualSlot) = -ay;
    } else if (axisX == SDL_CONTROLLER_AXIS_RIGHTX) {
        getRightStickX(virtualSlot) = +ax;
        getRightStickY(virtualSlot) = -ay;
    }
}

int32_t SDLController::ReadRawPress() {
    SDL_GameControllerUpdate();

    for (int32_t i = SDL_CONTROLLER_BUTTON_A; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        if (SDL_GameControllerGetButton(mController, static_cast<SDL_GameControllerButton>(i))) {
            return i;
        }
    }

    for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
        const auto axis = static_cast<SDL_GameControllerAxis>(i);
        const auto axisValue = SDL_GameControllerGetAxis(mController, axis) / 32767.0f;

        if (axisValue < -0.7f) {
            return -(axis + AXIS_SCANCODE_BIT);
        }

        if (axisValue > 0.7f) {
            return (axis + AXIS_SCANCODE_BIT);
        }
    }

    return -1;
}

void SDLController::ReadFromSource(int32_t virtualSlot) {
    auto profile = getProfile(virtualSlot);

    SDL_GameControllerUpdate();

    // If the controller is disconnected, close it.
    if (mController != nullptr && !SDL_GameControllerGetAttached(mController)) {
        Close();
    }

    // Attempt to load the controller if it's not loaded
    if (mController == nullptr) {
        // If we failed to load the controller, don't process it.
        if (!Open()) {
            return;
        }
    }

    if (mSupportsGyro && profile->UseGyro) {

        float gyroData[3];
        SDL_GameControllerGetSensorData(mController, SDL_SENSOR_GYRO, gyroData, 3);

        float gyroDriftX = profile->GyroData[DRIFT_X] / 100.0f;
        float gyroDriftY = profile->GyroData[DRIFT_Y] / 100.0f;
        const float gyro_sensitivity = profile->GyroData[GYRO_SENSITIVITY];

        if (gyroDriftX == 0) {
            gyroDriftX = gyroData[0];
        }

        if (gyroDriftY == 0) {
            gyroDriftY = gyroData[1];
        }

        profile->GyroData[DRIFT_X] = gyroDriftX * 100.0f;
        profile->GyroData[DRIFT_Y] = gyroDriftY * 100.0f;

        getGyroX(virtualSlot) = gyroData[0] - gyroDriftX;
        getGyroY(virtualSlot) = gyroData[1] - gyroDriftY;

        getGyroX(virtualSlot) *= gyro_sensitivity;
        getGyroY(virtualSlot) *= gyro_sensitivity;
    } else {
        getGyroX(virtualSlot) = 0;
        getGyroY(virtualSlot) = 0;
    }

    getPressedButtons(virtualSlot) = 0;

    for (int32_t i = SDL_CONTROLLER_BUTTON_A; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        if (profile->Mappings.contains(i)) {
            if (SDL_GameControllerGetButton(mController, static_cast<SDL_GameControllerButton>(i))) {
                getPressedButtons(virtualSlot) |= profile->Mappings[i];
            } else {
                getPressedButtons(virtualSlot) &= ~profile->Mappings[i];
            }
        }
    }

    SDL_GameControllerAxis leftStickAxisX = SDL_CONTROLLER_AXIS_INVALID;
    SDL_GameControllerAxis leftStickAxisY = SDL_CONTROLLER_AXIS_INVALID;
    int32_t leftStickDeadzone = 0;

    SDL_GameControllerAxis rightStickAxisX = SDL_CONTROLLER_AXIS_INVALID;
    SDL_GameControllerAxis rightStickAxisY = SDL_CONTROLLER_AXIS_INVALID;
    int32_t rightStickDeadzone = 0;

    for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
        const auto axis = static_cast<SDL_GameControllerAxis>(i);
        const auto posScancode = i | AXIS_SCANCODE_BIT;
        const auto negScancode = -posScancode;
        const auto axisDeadzone = profile->AxisDeadzones[i];
        const auto axisMinimumPress = profile->AxisMinimumPress[i];
        const auto posButton = profile->Mappings[posScancode];
        const auto negButton = profile->Mappings[negScancode];
        const auto axisValue = SDL_GameControllerGetAxis(mController, axis);

#ifdef TARGET_WEB
        // Firefox has a bug: https://bugzilla.mozilla.org/show_bug.cgi?id=1606562
        // It sets down y to 32768.0f / 32767.0f, which is greater than the allowed 1.0f,
        // which SDL then converts to a int16_t by multiplying by 32767.0f, which overflows into -32768.
        // Maximum up will hence never become -32768 with the current version of SDL2,
        // so this workaround should be safe in compliant browsers.
        if (AxisValue == -32768) {
            AxisValue = 32767;
        }
#endif

        if (!(posButton == BTN_STICKLEFT || posButton == BTN_STICKRIGHT || posButton == BTN_STICKUP ||
              posButton == BTN_STICKDOWN || negButton == BTN_STICKLEFT || negButton == BTN_STICKRIGHT ||
              negButton == BTN_STICKUP || negButton == BTN_STICKDOWN || posButton == BTN_VSTICKLEFT ||
              posButton == BTN_VSTICKRIGHT || posButton == BTN_VSTICKUP || posButton == BTN_VSTICKDOWN ||
              negButton == BTN_VSTICKLEFT || negButton == BTN_VSTICKRIGHT || negButton == BTN_VSTICKUP ||
              negButton == BTN_VSTICKDOWN)) {

            // The axis is being treated as a "button"
            if (axisValue > axisMinimumPress) {
                getPressedButtons(virtualSlot) |= posButton;
                getPressedButtons(virtualSlot) &= ~negButton;
            } else if (axisValue < -axisMinimumPress) {
                getPressedButtons(virtualSlot) &= ~posButton;
                getPressedButtons(virtualSlot) |= negButton;
            } else {
                getPressedButtons(virtualSlot) &= ~posButton;
                getPressedButtons(virtualSlot) &= ~negButton;
            }
        } else {
            // The axis is being treated as a "stick"

            // Left stick
            if (posButton == BTN_STICKLEFT || posButton == BTN_STICKRIGHT) {
                if (leftStickAxisX != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid PosStickX configured. Neg was {} and Pos is {}", LStickAxisX, Axis);
                }

                if (leftStickDeadzone != 0 && leftStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone configured. Up/Down was {} and Left/Right is {}", LStickDeadzone,
                                 AxisDeadzone);
                }

                leftStickDeadzone = axisDeadzone;
                leftStickAxisX = axis;
            }

            if (posButton == BTN_STICKUP || posButton == BTN_STICKDOWN) {
                if (leftStickAxisY != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid PosStickY configured. Neg was {} and Pos is {}", LStickAxisY, Axis);
                }

                if (leftStickDeadzone != 0 && leftStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone configured. Left/Right was {} and Up/Down is {}", LStickDeadzone,
                                 AxisDeadzone);
                }

                leftStickDeadzone = axisDeadzone;
                leftStickAxisY = axis;
            }

            if (negButton == BTN_STICKLEFT || negButton == BTN_STICKRIGHT) {
                if (leftStickAxisX != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid NegStickX configured. Pos was {} and Neg is {}", LStickAxisX, Axis);
                }

                if (leftStickDeadzone != 0 && leftStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone configured. Left/Right was {} and Up/Down is {}", LStickDeadzone,
                                 AxisDeadzone);
                }

                leftStickDeadzone = axisDeadzone;
                leftStickAxisX = axis;
            }

            if (negButton == BTN_STICKUP || negButton == BTN_STICKDOWN) {
                if (leftStickAxisY != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid NegStickY configured. Pos was {} and Neg is {}", LStickAxisY, Axis);
                }

                if (leftStickDeadzone != 0 && leftStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone misconfigured. Left/Right was {} and Up/Down is {}", LStickDeadzone,
                                 AxisDeadzone);
                }

                leftStickDeadzone = axisDeadzone;
                leftStickAxisY = axis;
            }

            // Right Stick
            if (posButton == BTN_VSTICKLEFT || posButton == BTN_VSTICKRIGHT) {
                if (rightStickAxisX != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid PosStickX configured. Neg was {} and Pos is {}", RStickAxisX, Axis);
                }

                if (rightStickDeadzone != 0 && rightStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone configured. Up/Down was {} and Left/Right is {}", RStickDeadzone,
                                 AxisDeadzone);
                }

                rightStickDeadzone = axisDeadzone;
                rightStickAxisX = axis;
            }

            if (posButton == BTN_VSTICKUP || posButton == BTN_VSTICKDOWN) {
                if (rightStickAxisY != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid PosStickY configured. Neg was {} and Pos is {}", RStickAxisY, Axis);
                }

                if (rightStickDeadzone != 0 && rightStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone configured. Left/Right was {} and Up/Down is {}", RStickDeadzone,
                                 AxisDeadzone);
                }

                rightStickDeadzone = axisDeadzone;
                rightStickAxisY = axis;
            }

            if (negButton == BTN_VSTICKLEFT || negButton == BTN_VSTICKRIGHT) {
                if (rightStickAxisX != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid NegStickX configured. Pos was {} and Neg is {}", RStickAxisX, Axis);
                }

                if (rightStickDeadzone != 0 && rightStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone configured. Left/Right was {} and Up/Down is {}", RStickDeadzone,
                                 AxisDeadzone);
                }

                rightStickDeadzone = axisDeadzone;
                rightStickAxisX = axis;
            }

            if (negButton == BTN_VSTICKUP || negButton == BTN_VSTICKDOWN) {
                if (rightStickAxisY != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid NegStickY configured. Pos was {} and Neg is {}", RStickAxisY, Axis);
                }

                if (rightStickDeadzone != 0 && rightStickDeadzone != axisDeadzone) {
                    SPDLOG_TRACE("Invalid Deadzone misconfigured. Left/Right was {} and Up/Down is {}", RStickDeadzone,
                                 AxisDeadzone);
                }

                rightStickDeadzone = axisDeadzone;
                rightStickAxisY = axis;
            }
        }
    }

    if (leftStickAxisX != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisY != SDL_CONTROLLER_AXIS_INVALID) {
        NormalizeStickAxis(leftStickAxisX, leftStickAxisY, leftStickDeadzone, virtualSlot);
    }

    if (rightStickAxisX != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisY != SDL_CONTROLLER_AXIS_INVALID) {
        NormalizeStickAxis(rightStickAxisX, rightStickAxisY, rightStickDeadzone, virtualSlot);
    }
}

void SDLController::WriteToSource(int32_t virtualSlot, ControllerCallback* controller) {
    if (CanRumble() && getProfile(virtualSlot)->UseRumble) {
        if (controller->rumble > 0) {
            float rumble_strength = getProfile(virtualSlot)->RumbleStrength;
            SDL_GameControllerRumble(mController, 0xFFFF * rumble_strength, 0xFFFF * rumble_strength, 0);
        } else {
            SDL_GameControllerRumble(mController, 0, 0, 0);
        }
    }

    if (SDL_GameControllerHasLED(mController)) {
        switch (controller->ledColor) {
            case 0:
                SDL_JoystickSetLED(SDL_GameControllerGetJoystick(mController), 255, 0, 0);
                break;
            case 1:
                SDL_JoystickSetLED(SDL_GameControllerGetJoystick(mController), 0x1E, 0x69, 0x1B);
                break;
            case 2:
                SDL_JoystickSetLED(SDL_GameControllerGetJoystick(mController), 0x64, 0x14, 0x00);
                break;
            case 3:
                SDL_JoystickSetLED(SDL_GameControllerGetJoystick(mController), 0x00, 0x3C, 0x64);
                break;
        }
    }
}

const std::string SDLController::GetButtonName(int32_t virtualSlot, int32_t n64Button) {
    char buffer[50];
    std::map<int32_t, int32_t>& mappings = getProfile(virtualSlot)->Mappings;

    const auto find =
        std::find_if(mappings.begin(), mappings.end(),
                     [n64Button](const std::pair<int32_t, int32_t>& pair) { return pair.second == n64Button; });

    if (find == mappings.end()) {
        return "Unknown";
    }

    int btn = abs(find->first);

    if (btn >= AXIS_SCANCODE_BIT) {
        btn -= AXIS_SCANCODE_BIT;

        snprintf(buffer, sizeof(buffer), "%s%s", AxisNames[btn], find->first > 0 ? "+" : "-");
        return buffer;
    }

    snprintf(buffer, sizeof(buffer), "Button %d", btn);
    return buffer;
}

const std::string SDLController::GetControllerName() {
    return mControllerName;
}

void SDLController::CreateDefaultBinding(int32_t virtualSlot) {
    auto profile = getProfile(virtualSlot);
    profile->Mappings.clear();
    profile->AxisDeadzones.clear();
    profile->AxisMinimumPress.clear();
    profile->GyroData.clear();

    profile->Version = DEVICE_PROFILE_CURRENT_VERSION;
    profile->UseRumble = true;
    profile->RumbleStrength = 1.0f;
    profile->UseGyro = false;

    profile->Mappings[SDL_CONTROLLER_AXIS_RIGHTX | AXIS_SCANCODE_BIT] = BTN_CRIGHT;
    profile->Mappings[-(SDL_CONTROLLER_AXIS_RIGHTX | AXIS_SCANCODE_BIT)] = BTN_CLEFT;
    profile->Mappings[SDL_CONTROLLER_AXIS_RIGHTY | AXIS_SCANCODE_BIT] = BTN_CDOWN;
    profile->Mappings[-(SDL_CONTROLLER_AXIS_RIGHTY | AXIS_SCANCODE_BIT)] = BTN_CUP;
    profile->Mappings[SDL_CONTROLLER_AXIS_LEFTX | AXIS_SCANCODE_BIT] = BTN_STICKRIGHT;
    profile->Mappings[-(SDL_CONTROLLER_AXIS_LEFTX | AXIS_SCANCODE_BIT)] = BTN_STICKLEFT;
    profile->Mappings[SDL_CONTROLLER_AXIS_LEFTY | AXIS_SCANCODE_BIT] = BTN_STICKDOWN;
    profile->Mappings[-(SDL_CONTROLLER_AXIS_LEFTY | AXIS_SCANCODE_BIT)] = BTN_STICKUP;
    profile->Mappings[SDL_CONTROLLER_AXIS_TRIGGERRIGHT | AXIS_SCANCODE_BIT] = BTN_R;
    profile->Mappings[SDL_CONTROLLER_AXIS_TRIGGERLEFT | AXIS_SCANCODE_BIT] = BTN_Z;
    profile->Mappings[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = BTN_L;
    profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = BTN_DRIGHT;
    profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = BTN_DLEFT;
    profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = BTN_DDOWN;
    profile->Mappings[SDL_CONTROLLER_BUTTON_DPAD_UP] = BTN_DUP;
    profile->Mappings[SDL_CONTROLLER_BUTTON_START] = BTN_START;
    profile->Mappings[SDL_CONTROLLER_BUTTON_B] = BTN_B;
    profile->Mappings[SDL_CONTROLLER_BUTTON_A] = BTN_A;
    profile->Mappings[SDL_CONTROLLER_BUTTON_LEFTSTICK] = BTN_MODIFIER1;
    profile->Mappings[SDL_CONTROLLER_BUTTON_RIGHTSTICK] = BTN_MODIFIER2;

    for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
        profile->AxisDeadzones[i] = 16.0f;
        profile->AxisMinimumPress[i] = 7680.0f;
    }

    profile->GyroData[DRIFT_X] = 0.0f;
    profile->GyroData[DRIFT_Y] = 0.0f;
    profile->GyroData[GYRO_SENSITIVITY] = 1.0f;
}

bool SDLController::Connected() const {
    return mController != nullptr;
}

bool SDLController::CanGyro() const {
    return mSupportsGyro;
}

bool SDLController::CanRumble() const {
#if SDL_COMPILEDVERSION >= SDL_VERSIONNUM(2, 0, 18)
    return SDL_GameControllerHasRumble(mController);
#endif
    return false;
}

void SDLController::ClearRawPress() {
}
} // namespace Ship
