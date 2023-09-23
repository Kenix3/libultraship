#include "SDLAxisDirectionToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLAxisDirectionToAxisDirectionMapping::SDLAxisDirectionToAxisDirectionMapping(int32_t sdlControllerIndex,
                                                                               int32_t sdlControllerAxis,
                                                                               int32_t axisDirection)
    : SDLMapping(sdlControllerIndex) {
    mControllerAxis = static_cast<SDL_GameControllerAxis>(sdlControllerAxis);
    mAxisDirection = static_cast<AxisDirection>(axisDirection);
}
} // namespace LUS



    for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
        const auto axis = static_cast<SDL_GameControllerAxis>(i);
        const auto posScancode = i | AXIS_SCANCODE_BIT;
        const auto negScancode = -posScancode;
        const auto axisMinimumPress = profile->AxisMinimumPress[i];
        const auto axisDeadzone = profile->AxisDeadzones[i];
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

            // The axis is being treated as a "stick"

            // Left stick
            if (posButton == BTN_STICKLEFT || posButton == BTN_STICKRIGHT) {
                if (leftStickAxisX != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid PosStickX configured. Neg was {} and Pos is {}", leftStickAxisX, axis);
                }

                leftStickAxisX = axis;
            }

            if (posButton == BTN_STICKUP || posButton == BTN_STICKDOWN) {
                if (leftStickAxisY != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid PosStickY configured. Neg was {} and Pos is {}", leftStickAxisY, axis);
                }

                leftStickAxisY = axis;
            }

            if (negButton == BTN_STICKLEFT || negButton == BTN_STICKRIGHT) {
                if (leftStickAxisX != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid NegStickX configured. Pos was {} and Neg is {}", leftStickAxisX, axis);
                }

                leftStickAxisX = axis;
            }

            if (negButton == BTN_STICKUP || negButton == BTN_STICKDOWN) {
                if (leftStickAxisY != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid NegStickY configured. Pos was {} and Neg is {}", leftStickAxisY, axis);
                }

                leftStickAxisY = axis;
            }

            // Right Stick
            if (posButton == BTN_VSTICKLEFT || posButton == BTN_VSTICKRIGHT) {
                if (rightStickAxisX != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid PosStickX configured. Neg was {} and Pos is {}", rightStickAxisX, axis);
                }

                rightStickAxisX = axis;
            }

            if (posButton == BTN_VSTICKUP || posButton == BTN_VSTICKDOWN) {
                if (rightStickAxisY != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid PosStickY configured. Neg was {} and Pos is {}", rightStickAxisY, axis);
                }

                rightStickAxisY = axis;
            }

            if (negButton == BTN_VSTICKLEFT || negButton == BTN_VSTICKRIGHT) {
                if (rightStickAxisX != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisX != axis) {
                    SPDLOG_TRACE("Invalid NegStickX configured. Pos was {} and Neg is {}", rightStickAxisX, axis);
                }

                rightStickAxisX = axis;
            }

            if (negButton == BTN_VSTICKUP || negButton == BTN_VSTICKDOWN) {
                if (rightStickAxisY != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisY != axis) {
                    SPDLOG_TRACE("Invalid NegStickY configured. Pos was {} and Neg is {}", rightStickAxisY, axis);
                }

                rightStickAxisY = axis;
            }
        }
    }

    if (leftStickAxisX != SDL_CONTROLLER_AXIS_INVALID && leftStickAxisY != SDL_CONTROLLER_AXIS_INVALID) {
        NormalizeStickAxis(leftStickAxisX, leftStickAxisY, portIndex);
    }

    if (rightStickAxisX != SDL_CONTROLLER_AXIS_INVALID && rightStickAxisY != SDL_CONTROLLER_AXIS_INVALID) {
        NormalizeStickAxis(rightStickAxisX, rightStickAxisY, portIndex);
    }

    void SDLController::NormalizeStickAxis(SDL_GameControllerAxis axisX, SDL_GameControllerAxis axisY, int32_t portIndex)
{
    const auto axisValueX = SDL_GameControllerGetAxis(mController, axisX);
    const auto axisValueY = SDL_GameControllerGetAxis(mController, axisY);

    auto ax = NormaliseStickValue(axisValueX);
    auto ay = NormaliseStickValue(axisValueY);

    if (axisX == SDL_CONTROLLER_AXIS_LEFTX) {
        GetLeftStickX(portIndex) = +ax;
        GetLeftStickY(portIndex) = -ay;
    } else if (axisX == SDL_CONTROLLER_AXIS_RIGHTX) {
        GetRightStickX(portIndex) = +ax;
        GetRightStickY(portIndex) = -ay;
    }
}

float SDLController::NormaliseStickValue(float axisValue) {
    // scale {-32768 ... +32767} to {-MAX_AXIS_RANGE ... +MAX_AXIS_RANGE}
    return axisValue * MAX_AXIS_RANGE / MAX_SDL_RANGE;
}