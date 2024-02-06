#include "ControllerGyroMapping.h"

namespace LUS {
ControllerGyroMapping::ControllerGyroMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, float sensitivity)
    : ControllerInputMapping(lusDeviceIndex), mPortIndex(portIndex), mSensitivity(sensitivity) {
    mSensitivityPercent = mSensitivity * 100;
}

ControllerGyroMapping::~ControllerGyroMapping() {
}

void ControllerGyroMapping::SetSensitivity(uint8_t sensitivityPercent) {
    mSensitivityPercent = sensitivityPercent;
    mSensitivity = sensitivityPercent / 100.0f;
    SaveToConfig();
}

uint8_t ControllerGyroMapping::GetSensitivityPercent() {
    return mSensitivityPercent;
}

float ControllerGyroMapping::GetSensitivity() {
    return mSensitivity;
}

void ControllerGyroMapping::ResetSensitivityToDefault() {
    SetSensitivity(GYRO_SENSITIVITY_DEFAULT);
}

bool ControllerGyroMapping::SensitivityIsDefault() {
    return mSensitivityPercent == GYRO_SENSITIVITY_DEFAULT;
}

void ControllerGyroMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace LUS
