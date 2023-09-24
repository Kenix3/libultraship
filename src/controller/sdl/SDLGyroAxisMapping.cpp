#include "SDLGyroAxisMapping.h"
#include <spdlog/spdlog.h>

namespace LUS {
SDLGyroAxisMapping::SDLGyroAxisMapping(int32_t sdlControllerIndex, int32_t axis) : SDLMapping(sdlControllerIndex) {
    mAxis = static_cast<Axis>(axis);
}

float SDLGyroAxisMapping::GetGyroAxisValue() {
    if (!ControllerLoaded()) {
        return 0.0f;
    }

    float gyroData[3] = { 0 };
    if (SDL_GameControllerGetSensorData(mController, SDL_SENSOR_GYRO, gyroData, 3) == -1) {
        return 0.0f;
    };

    return gyroData[mAxis];

    // todo: figure out drift and sensitivity stuff
    // i can probably wing it but i'd rather be able to test it
    // and sdl doesn't see any of my controllers as having gyro

    // float gyroDriftX = profile->GyroData[DRIFT_X] / 100.0f;
    // float gyroDriftY = profile->GyroData[DRIFT_Y] / 100.0f;
    // const float gyroSensitivity = profile->GyroData[GYRO_SENSITIVITY];

    // if (gyroDriftX == 0) {
    //     gyroDriftX = gyroData[0];
    // }

    // if (gyroDriftY == 0) {
    //     gyroDriftY = gyroData[1];
    // }

    // profile->GyroData[DRIFT_X] = gyroDriftX * 100.0f;
    // profile->GyroData[DRIFT_Y] = gyroDriftY * 100.0f;

    // GetGyroX(portIndex) = gyroData[0] - gyroDriftX;
    // GetGyroY(portIndex) = gyroData[1] - gyroDriftY;

    // GetGyroX(portIndex) *= gyroSensitivity;
    // GetGyroY(portIndex) *= gyroSensitivity;
}
} // namespace LUS
