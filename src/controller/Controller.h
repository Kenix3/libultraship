#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include <vector>
#include "libultraship/libultra/controller.h"
#include "libultraship/color.h"
#include <unordered_map>
#include "ButtonMapping.h"

#define EXTENDED_SCANCODE_BIT (1 << 8)
#define AXIS_SCANCODE_BIT (1 << 9)
#define MAX_AXIS_RANGE 85.0f

namespace LUS {
enum GyroData { DRIFT_X, DRIFT_Y, GYRO_SENSITIVITY };
enum Stick { LEFT, RIGHT };
enum Axis { X, Y };
enum DeviceProfileVersion {
    DEVICE_PROFILE_VERSION_0 = 0,
    DEVICE_PROFILE_VERSION_1 = 1,
    DEVICE_PROFILE_VERSION_2 = 2,
    DEVICE_PROFILE_VERSION_3 = 3
};

#define DEVICE_PROFILE_CURRENT_VERSION DEVICE_PROFILE_VERSION_3

struct DeviceProfile {
    int32_t Version = 0;
    bool UseRumble = false;
    bool UseGyro = false;
    bool UseStickDeadzoneForButtons = false;
    float RumbleStrength = 1.0f;
    int32_t NotchProximityThreshold = 0;
    std::unordered_map<int32_t, float> AxisDeadzones;
    std::unordered_map<int32_t, float> AxisMinimumPress;
    std::unordered_map<int32_t, float> GyroData;
    std::map<int32_t, int32_t> Mappings;
};

class Controller {
  public:
    Controller();
    ~Controller();
    int32_t GetConnectedPortIndex();
    void ReadToPad(OSContPad* pad);

  protected:
    int32_t mConnectedPortIndex;

    int8_t ReadStick(int32_t portIndex, Stick stick, Axis axis);
    void ProcessStick(int8_t& x, int8_t& y, float deadzoneX, float deadzoneY, int32_t notchProxmityThreshold);
    double GetClosestNotch(double angle, double approximationThreshold);

  private:
    std::vector<ButtonMapping> mButtonMappings;

    struct Buttons {
        int32_t PressedButtons = 0;
        int8_t LeftStickX = 0;
        int8_t LeftStickY = 0;
        int8_t RightStickX = 0;
        int8_t RightStickY = 0;
        float GyroX = 0.0f;
        float GyroY = 0.0f;
    };

    std::unordered_map<int32_t, std::shared_ptr<DeviceProfile>> mProfiles;
    std::unordered_map<int32_t, std::shared_ptr<Buttons>> mButtonData = {};
    std::deque<OSContPad> mPadBuffer;
};
} // namespace LUS
