#ifdef __WIIU__
#include "WiiUGyroMapping.h"
#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"

#include "public/bridge/consolevariablebridge.h"
#include <Utils/StringHelper.h>

namespace LUS {
WiiUGyroMapping::WiiUGyroMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, float sensitivity,
                                 float neutralPitch, float neutralYaw, float neutralRoll)
    : ControllerInputMapping(lusDeviceIndex), ControllerGyroMapping(lusDeviceIndex, portIndex, sensitivity),
      mNeutralPitch(neutralPitch), WiiUMapping(lusDeviceIndex), mNeutralYaw(neutralYaw), mNeutralRoll(neutralRoll) {
}

void WiiUGyroMapping::Recalibrate() {
    if (!IsGamepad()) {
        mNeutralPitch = 0;
        mNeutralYaw = 0;
        mNeutralRoll = 0;
        return;
    }

    VPADReadError error;
    VPADStatus* status = LUS::WiiU::GetVPADStatus(&error);
    if (status == nullptr) {
        mNeutralPitch = 0;
        mNeutralYaw = 0;
        mNeutralRoll = 0;
        return;
    }

    mNeutralPitch = status->gyro.x * -8.0f;
    mNeutralYaw = status->gyro.z * 8.0f;
    mNeutralRoll = status->gyro.y * 8.0f;
}

void WiiUGyroMapping::UpdatePad(float& x, float& y) {
    if (!IsGamepad() || Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        x = 0;
        y = 0;
        return;
    }

    VPADReadError error;
    VPADStatus* status = LUS::WiiU::GetVPADStatus(&error);
    if (status == nullptr) {
        x = 0;
        y = 0;
        return;
    }

    x = ((status->gyro.x * -8.0f) - mNeutralPitch) * mSensitivity;
    y = ((status->gyro.z * 8.0f) - mNeutralYaw) * mSensitivity;
}

std::string WiiUGyroMapping::GetGyroMappingId() {
    return StringHelper::Sprintf("P%d-LUSI%d", mPortIndex, ControllerInputMapping::mLUSDeviceIndex);
}

void WiiUGyroMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.GyroMappings." + GetGyroMappingId();

    CVarSetString(StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str(), "WiiUGyroMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mLUSDeviceIndex);
    CVarSetFloat(StringHelper::Sprintf("%s.Sensitivity", mappingCvarKey.c_str()).c_str(), mSensitivity);
    CVarSetFloat(StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str(), mNeutralPitch);
    CVarSetFloat(StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str(), mNeutralYaw);
    CVarSetFloat(StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str(), mNeutralRoll);

    CVarSave();
}

void WiiUGyroMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.GyroMappings." + GetGyroMappingId();

    CVarClear(StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.LUSDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Sensitivity", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str());

    CVarSave();
}

std::string WiiUGyroMapping::GetPhysicalDeviceName() {
    return GetWiiUDeviceName();
}

bool WiiUGyroMapping::PhysicalDeviceIsConnected() {
    return WiiUDeviceIsConnected();
}
} // namespace LUS
#endif
