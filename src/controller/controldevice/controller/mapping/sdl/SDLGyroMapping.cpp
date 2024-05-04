#include "SDLGyroMapping.h"
#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"

#include "public/bridge/consolevariablebridge.h"
#include "utils/StringHelper.h"

namespace Ship {
SDLGyroMapping::SDLGyroMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, float sensitivity,
                               float neutralPitch, float neutralYaw, float neutralRoll)
    : ControllerInputMapping(shipDeviceIndex), ControllerGyroMapping(shipDeviceIndex, portIndex, sensitivity),
      mNeutralPitch(neutralPitch), SDLMapping(shipDeviceIndex), mNeutralYaw(neutralYaw), mNeutralRoll(neutralRoll) {
}

void SDLGyroMapping::Recalibrate() {
    if (!ControllerLoaded()) {
        mNeutralPitch = 0;
        mNeutralYaw = 0;
        mNeutralRoll = 0;
        return;
    }

    float gyroData[3];
    SDL_GameControllerSetSensorEnabled(mController, SDL_SENSOR_GYRO, SDL_TRUE);
    SDL_GameControllerGetSensorData(mController, SDL_SENSOR_GYRO, gyroData, 3);

    mNeutralPitch = gyroData[0];
    mNeutralYaw = gyroData[1];
    mNeutralRoll = gyroData[2];
}

void SDLGyroMapping::UpdatePad(float& x, float& y) {
    if (!ControllerLoaded() || Context::GetInstance()->GetControlDeck()->GamepadGameInputBlocked()) {
        x = 0;
        y = 0;
        return;
    }

    float gyroData[3];
    SDL_GameControllerSetSensorEnabled(mController, SDL_SENSOR_GYRO, SDL_TRUE);
    SDL_GameControllerGetSensorData(mController, SDL_SENSOR_GYRO, gyroData, 3);

    x = (gyroData[0] - mNeutralPitch) * mSensitivity;
    y = (gyroData[1] - mNeutralYaw) * mSensitivity;
}

std::string SDLGyroMapping::GetGyroMappingId() {
    return StringHelper::Sprintf("P%d-LUSI%d", mPortIndex, ControllerInputMapping::mShipDeviceIndex);
}

void SDLGyroMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GyroMappings." + GetGyroMappingId();

    CVarSetString(StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str(), "SDLGyroMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str(),
                   ControllerInputMapping::mShipDeviceIndex);
    CVarSetFloat(StringHelper::Sprintf("%s.Sensitivity", mappingCvarKey.c_str()).c_str(), mSensitivity);
    CVarSetFloat(StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str(), mNeutralPitch);
    CVarSetFloat(StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str(), mNeutralYaw);
    CVarSetFloat(StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str(), mNeutralRoll);

    CVarSave();
}

void SDLGyroMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".GyroMappings." + GetGyroMappingId();

    CVarClear(StringHelper::Sprintf("%s.GyroMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.Sensitivity", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.NeutralPitch", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.NeutralYaw", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.NeutralRoll", mappingCvarKey.c_str()).c_str());

    CVarSave();
}

std::string SDLGyroMapping::GetPhysicalDeviceName() {
    return GetSDLDeviceName();
}

bool SDLGyroMapping::PhysicalDeviceIsConnected() {
    return ControllerLoaded();
}
} // namespace Ship
