#include "TASController.h"
#include "Utils/StringHelper.h"

static FILE *fp;

namespace LUS {
TASController::TASController(int32_t deviceIndex)
    : Controller(deviceIndex) {
    mGuid = StringHelper::Sprintf("TASController%d", deviceIndex);
    mControllerName = "TASController";
    mButtonName = "TAS Btn";

    fp = fopen("cont.m64", "rb");
    if (fp != NULL) {
        uint8_t buf[0x400];
        fread(buf, 1, sizeof(buf), fp);
    }
}

void TASController::ReadToPad(OSContPad* pad, int32_t portIndex) {
    if (pad == nullptr || fp == nullptr) {
        return;
    }

    uint8_t bytes[4] = {0};
    fread(bytes, 1, 4, fp);
    pad->button = (bytes[0] << 8) | bytes[1];
    pad->stick_x = bytes[2];
    pad->stick_y = bytes[3];
}

void TASController::ReadDevice(int32_t portIndex) {
}

const std::string TASController::GetButtonName(int32_t portIndex, int32_t n64bitmask) {
    return mButtonName;
}

bool TASController::Connected() const {
    return true;
}

int32_t TASController::SetRumble(int32_t portIndex, bool rumble) {
    return -1000;
}

int32_t TASController::SetLedColor(int32_t portIndex, Color_RGB8 color) {
    return -1000;
}

bool TASController::CanSetLed() const {
    return false;
}

bool TASController::CanRumble() const {
    return false;
}

bool TASController::CanGyro() const {
    return false;
}

void TASController::CreateDefaultBinding(int32_t portIndex) {
    auto profile = GetProfile(portIndex);
    profile->Mappings.clear();
    profile->AxisDeadzones.clear();
    profile->AxisMinimumPress.clear();
    profile->GyroData.clear();

    profile->Version = DEVICE_PROFILE_CURRENT_VERSION;
    profile->UseRumble = false;
    profile->RumbleStrength = 0.0f;
    profile->UseGyro = false;

    for (int32_t i = 0; i < 6; i++) {
        profile->AxisDeadzones[i] = 0.0f;
        profile->AxisMinimumPress[i] = 0.0f;
    }

    profile->GyroData[DRIFT_X] = 0.0f;
    profile->GyroData[DRIFT_Y] = 0.0f;
    profile->GyroData[GYRO_SENSITIVITY] = 0.0f;
}

void TASController::ClearRawPress() {
}

int32_t TASController::ReadRawPress() {
    return -1;
}
} // namespace LUS