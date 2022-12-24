#ifdef __SWITCH__
#include "SwitchController.h"
#include "core/Window.h"
#include "menu/ImGuiImpl.h"
#include "SwitchImpl.h"
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>

namespace Ship {

SwitchController::SwitchController(int32_t physicalSlot) : Controller(), mPhysicalSlot(physicalSlot) {
    mConnected = false;
    mControllerName = "Switch Controller #" + std::to_string(mPhysicalSlot) + " (Disconnected)";
}

bool SwitchController::Open() {
    mConnected = true;
    mController = new NXControllerState();
    padInitializeDefault(&mController->State);

    // Initialize vibration devices

    hidInitializeVibrationDevices(mController->Handles[1], 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);
    hidInitializeVibrationDevices(mController->Handles[0], 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);

    // Initialize six axis sensors

    hidGetSixAxisSensorHandles(&mController->Sensors[0], 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidGetSixAxisSensorHandles(&mController->Sensors[1], 1, HidNpadIdType_No1, HidNpadStyleTag_NpadFullKey);
    hidGetSixAxisSensorHandles(&mController->Sensors[2], 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);

    for (int i = 0; i < 4; i++) {
        hidStartSixAxisSensor(mController->Sensors[i]);
    }

    mGuid = StringHelper::Sprintf("NXInternal:%d", mPhysicalSlot);
    mControllerName = "Switch Controller #" + std::to_string(mPhysicalSlot) + " (" + GetControllerExtensionName() + ")";
    return true;
}

void SwitchController::Close() {
    mConnected = false;

    for (int i = 0; i < MAXCONTROLLERS; i++) {
        getPressedButtons(i) = 0;
        getLeftStickX(i) = 0;
        getLeftStickY(i) = 0;
        getRightStickX(i) = 0;
        getRightStickY(i) = 0;
        getGyroX(i) = 0;
        getGyroY(i) = 0;
    }
}

void SwitchController::NormalizeStickAxis(int32_t virtualSlot, float x, float y, int16_t axisThreshold,
                                          bool isRightStick) {
    auto profile = getProfile(virtualSlot);

    auto ax = x * 85.0f / 32767.0f;
    auto ay = y * 85.0f / 32767.0f;

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

    if (isRightStick) {
        getRightStickX(virtualSlot) = +ax;
        getRightStickY(virtualSlot) = ay;
    } else {
        getLeftStickX(virtualSlot) = +ax;
        getLeftStickY(virtualSlot) = ay;
    }
}

void SwitchController::ReadFromSource(int32_t virtualSlot) {
    auto profile = getProfile(virtualSlot);
    PadState* state = &mController->State;
    padUpdate(state);

    const auto pressedBtns = padGetButtons(state);

    getPressedButtons(virtualSlot) = 0;
    getLeftStickX(virtualSlot) = 0;
    getLeftStickY(virtualSlot) = 0;
    getRightStickX(virtualSlot) = 0;
    getRightStickY(virtualSlot) = 0;
    getGyroX(virtualSlot) = 0;
    getGyroY(virtualSlot) = 0;

    int16_t stickX = 0;
    int16_t stickY = 0;
    int16_t camX = 0;
    int16_t camY = 0;
    for (uint32_t id = 0; id < 35; id++) {
        uint32_t btn = BIT(id);
        if (!profile->Mappings.contains(btn)) {
            continue;
        }

        const auto pBtn = profile->Mappings[btn];

        if (btn >= HidNpadButton_StickLLeft) {

            HidAnalogStickState lStick = padGetStickPos(state, 0);
            HidAnalogStickState rStick = padGetStickPos(state, 1);
            float axisX = btn >= HidNpadButton_StickRLeft ? rStick.x : lStick.x;
            float axisY = btn >= HidNpadButton_StickRLeft ? rStick.y : lStick.y;

            if (pBtn == BTN_STICKRIGHT || pBtn == BTN_STICKLEFT) {
                stickX = axisX;
                continue;
            } else if (pBtn == BTN_STICKDOWN || pBtn == BTN_STICKUP) {
                stickY = axisY;
                continue;
            } else if (pBtn == BTN_VSTICKRIGHT || pBtn == BTN_VSTICKLEFT) {
                camX = axisX;
                continue;
            } else if (pBtn == BTN_VSTICKDOWN || pBtn == BTN_VSTICKUP) {
                camY = axisY;
                continue;
            }
        }

        if (pressedBtns & btn) {
            getPressedButtons(virtualSlot) |= pBtn;
        } else {
            getPressedButtons(virtualSlot) &= ~pBtn;
        }
    }

    if (stickX || stickY) {
        NormalizeStickAxis(virtualSlot, stickX, stickY, profile->AxisDeadzones[0], false);
    }

    if (camX || camY) {
        NormalizeStickAxis(virtualSlot, camX, camY, profile->AxisDeadzones[2], true);
    }

    if (profile->UseGyro) {
        HidSixAxisSensorState sixaxis = { 0 };
        UpdateSixAxisSensor(sixaxis);

        float gyroX = sixaxis.angular_velocity.x * -8.0f;
        float gyroY = sixaxis.angular_velocity.y * -8.0f;

        float gyroDriftX = profile->GyroData[DRIFT_X] / 100.0f;
        float gyroDriftY = profile->GyroData[DRIFT_Y] / 100.0f;
        const float gyroSensitivity = profile->GyroData[GYRO_SENSITIVITY];

        if (gyroDriftX == 0) {
            gyroDriftX = gyroX;
        }

        if (gyroDriftY == 0) {
            gyroDriftY = gyroY;
        }

        profile->GyroData[DRIFT_X] = gyroDriftX * 100.0f;
        profile->GyroData[DRIFT_Y] = gyroDriftY * 100.0f;

        getGyroX(virtualSlot) = gyroX - gyroDriftX;
        getGyroY(virtualSlot) = gyroY - gyroDriftY;

        getGyroX(virtualSlot) *= gyroSensitivity;
        getGyroY(virtualSlot) *= gyroSensitivity;
    }
}

void SwitchController::WriteToSource(int32_t virtualSlot, ControllerCallback* controller) {
    if (getProfile(virtualSlot)->UseRumble) {
        PadState* state = &mController->State;
        float rumbleStrength = getProfile(virtualSlot)->RumbleStrength;
        HidVibrationValue vibrationValues[2];

        for (int i = 0; i < 2; i++) {
            float amp = controller->rumble > 0 ? 0xFFFF * rumbleStrength : 0.0f;
            vibrationValues[i].amp_low = amp;
            vibrationValues[i].amp_high = amp;
            vibrationValues[i].freq_low = 160.0f;
            vibrationValues[i].freq_high = 320.0f;
        }

        hidSendVibrationValues(mController->Handles[padIsHandheld(state) ? 0 : 1], vibrationValues, 2);
        padUpdate(state);
    }
}

int32_t SwitchController::ReadRawPress() {
    PadState* state = &mController->State;
    padUpdate(state);
    u64 kDown = padGetButtonsDown(state);
    for (uint32_t i = 0; i < 35; i++) {
        if (kDown & BIT(i)) {
            return BIT(i);
        }
    }

    return -1;
}

const std::string SwitchController::GetButtonName(int32_t virtualSlot, int n64Button) {
    std::map<int32_t, int32_t>& mappings = getProfile(virtualSlot)->Mappings;
    const auto find =
        std::find_if(mappings.begin(), mappings.end(),
                     [n64Button](const std::pair<int32_t, int32_t>& pair) { return pair.second == n64Button; });

    if (find == mappings.end()) {
        return "Unknown";
    }

    uint32_t btn = find->first;

    switch (btn) {
        case HidNpadButton_A:
            return "A";
        case HidNpadButton_B:
            return "B";
        case HidNpadButton_X:
            return "X";
        case HidNpadButton_Y:
            return "Y";
        case HidNpadButton_Left:
            return "D-pad Left";
        case HidNpadButton_Right:
            return "D-pad Right";
        case HidNpadButton_Up:
            return "D-pad Up";
        case HidNpadButton_Down:
            return "D-pad Down";
        case HidNpadButton_ZL:
            return "ZL";
        case HidNpadButton_ZR:
            return "ZR";
        case HidNpadButton_L:
            return "L";
        case HidNpadButton_R:
            return "R";
        case HidNpadButton_Plus:
            return "+ (START)";
        case HidNpadButton_Minus:
            return "- (SELECT)";
        case HidNpadButton_StickR:
            return "Stick Button R";
        case HidNpadButton_StickL:
            return "Stick Button L";
        case HidNpadButton_StickRLeft:
            return "Right Stick Left";
        case HidNpadButton_StickRRight:
            return "Right Stick Right";
        case HidNpadButton_StickRUp:
            return "Right Stick Up";
        case HidNpadButton_StickRDown:
            return "Right Stick Down";
        case HidNpadButton_StickLLeft:
            return "Left Stick Left";
        case HidNpadButton_StickLRight:
            return "Left Stick Right";
        case HidNpadButton_StickLUp:
            return "Left Stick Up";
        case HidNpadButton_StickLDown:
            return "Left Stick Down";
    }

    return "Unknown";
}

const std::string SwitchController::GetControllerName() {
    return mControllerName;
}

void SwitchController::CreateDefaultBinding(int32_t virtualSlot) {
    auto profile = getProfile(virtualSlot);
    profile->Mappings.clear();
    profile->AxisDeadzones.clear();
    profile->AxisMinimumPress.clear();
    profile->GyroData.clear();

    profile->Version = DEVICE_PROFILE_CURRENT_VERSION;
    profile->UseRumble = true;
    profile->RumbleStrength = 1.0f;
    profile->UseGyro = false;
    profile->Mappings[HidNpadButton_StickRRight] = BTN_CRIGHT;
    profile->Mappings[HidNpadButton_StickRLeft] = BTN_CLEFT;
    profile->Mappings[HidNpadButton_StickRDown] = BTN_CDOWN;
    profile->Mappings[HidNpadButton_StickRUp] = BTN_CUP;
    profile->Mappings[HidNpadButton_ZR] = BTN_R;
    profile->Mappings[HidNpadButton_L] = BTN_L;
    profile->Mappings[HidNpadButton_Right] = BTN_DRIGHT;
    profile->Mappings[HidNpadButton_Left] = BTN_DLEFT;
    profile->Mappings[HidNpadButton_Down] = BTN_DDOWN;
    profile->Mappings[HidNpadButton_Up] = BTN_DUP;
    profile->Mappings[HidNpadButton_Plus] = BTN_START;
    profile->Mappings[HidNpadButton_ZL] = BTN_Z;
    profile->Mappings[HidNpadButton_B] = BTN_B;
    profile->Mappings[HidNpadButton_A] = BTN_A;
    profile->Mappings[HidNpadButton_StickLRight] = BTN_STICKRIGHT;
    profile->Mappings[HidNpadButton_StickLLeft] = BTN_STICKLEFT;
    profile->Mappings[HidNpadButton_StickLDown] = BTN_STICKDOWN;
    profile->Mappings[HidNpadButton_StickLUp] = BTN_STICKUP;

    for (int i = 0; i < 4; i++) {
        profile->AxisDeadzones[i] = 16.0f;
        profile->AxisMinimumPress[i] = 7680.0f;
    }

    profile->GyroData[DRIFT_X] = 0.0f;
    profile->GyroData[DRIFT_Y] = 0.0f;
    profile->GyroData[GYRO_SENSITIVITY] = 1.0f;
}

void SwitchController::UpdateSixAxisSensor(HidSixAxisSensorState& state) {
    u64 styleSet = padGetStyleSet(&mController->State);
    if (styleSet & HidNpadStyleTag_NpadHandheld) {
        hidGetSixAxisSensorStates(mController->Sensors[0], &state, 1);
    } else if (styleSet & HidNpadStyleTag_NpadFullKey) {
        hidGetSixAxisSensorStates(mController->Sensors[1], &state, 1);
    } else if (styleSet & HidNpadStyleTag_NpadJoyDual) {
        u64 attrib = padGetAttributes(&mController->State);
        if (attrib & HidNpadAttribute_IsLeftConnected) {
            hidGetSixAxisSensorStates(mController->Sensors[2], &state, 1);
        } else if (attrib & HidNpadAttribute_IsRightConnected) {
            hidGetSixAxisSensorStates(mController->Sensors[3], &state, 1);
        }
    }
}

std::string SwitchController::GetControllerExtensionName() {
    u32 tagStyle = padGetStyleSet(&mController->State);

    if (tagStyle & HidNpadStyleTag_NpadFullKey) {
        return "Pro Controller";
    }

    if (tagStyle & HidNpadStyleTag_NpadHandheld) {
        return "Nintendo Switch";
    }

    if (tagStyle & HidNpadStyleTag_NpadJoyLeft) {
        return "Left Joy-Con";
    }

    if (tagStyle & HidNpadStyleTag_NpadJoyRight) {
        return "Right Joy-Con";
    }

    return "Dual Joy-Con";
}
} // namespace Ship
#endif