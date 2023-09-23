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
#include "ControllerStick.h"

#define EXTENDED_SCANCODE_BIT (1 << 8)
#define AXIS_SCANCODE_BIT (1 << 9)

namespace LUS {
enum GyroData { DRIFT_X, DRIFT_Y, GYRO_SENSITIVITY };


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
    bool IsConnected() const;
    void Connect();
    void Disconnect();
    void AddButtonMapping(std::shared_ptr<ButtonMapping> mapping);
    void ClearButtonMappings(uint16_t bitmask);
    void ClearAllButtonMappings();
    std::shared_ptr<ControllerStick> GetLeftStick();
    std::shared_ptr<ControllerStick> GetRightStick();
    void ReadToPad(OSContPad* pad);

  protected:
    // int8_t ReadStick(int32_t portIndex, Stick stick, Axis axis);
    // void ProcessStick(int8_t& x, int8_t& y, float deadzoneX, float deadzoneY, int32_t notchProxmityThreshold);
    // double GetClosestNotch(double angle, double approximationThreshold);

  private:
    std::vector<std::shared_ptr<ButtonMapping>> mButtonMappings;
    std::shared_ptr<ControllerStick> mLeftStick;
    std::shared_ptr<ControllerStick> mRightStick;

    bool mIsConnected;

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
