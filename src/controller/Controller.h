#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include "libultraship/libultra/controller.h"
#include "attachment/ControllerAttachment.h"
#include <unordered_map>

#define EXTENDED_SCANCODE_BIT (1 << 8)
#define AXIS_SCANCODE_BIT (1 << 9)
#define MAX_AXIS_RANGE 85.0f

namespace Ship {
enum GyroData { DRIFT_X, DRIFT_Y, GYRO_SENSITIVITY };
enum Stick { LEFT, RIGHT };
enum Axis { X, Y };
enum DeviceProfileVersion { DEVICE_PROFILE_VERSION_V0 = 0, DEVICE_PROFILE_VERSION_V1 = 1 };

#define DEVICE_PROFILE_CURRENT_VERSION DEVICE_PROFILE_VERSION_V1

struct DeviceProfile {
    int32_t Version = 0;
    bool UseRumble = false;
    bool UseGyro = false;
    float RumbleStrength = 1.0f;
    int32_t NotchProximityThreshold = 0;
    std::unordered_map<int32_t, float> AxisDeadzones;
    std::unordered_map<int32_t, float> AxisMinimumPress;
    std::unordered_map<int32_t, float> GyroData;
    std::map<int32_t, int32_t> Mappings;
};

class Controller {
  public:
    virtual ~Controller() = default;
    Controller();
    virtual void ReadFromSource(int32_t virtualSlot) = 0;
    virtual void WriteToSource(int32_t virtualSlot, ControllerCallback* controller) = 0;
    virtual bool Connected() const = 0;
    virtual bool CanRumble() const = 0;
    virtual bool CanGyro() const = 0;
    virtual void CreateDefaultBinding(int32_t virtualSlot) = 0;
    virtual void ClearRawPress() = 0;
    virtual int32_t ReadRawPress() = 0;
    virtual const std::string GetButtonName(int32_t virtualSlot, int32_t n64Button) = 0;
    virtual const std::string GetControllerName() = 0;
    void Read(OSContPad* pad, int32_t virtualSlot);
    void SetButtonMapping(int32_t virtualSlot, int32_t n64Button, int32_t scancode);
    std::shared_ptr<ControllerAttachment> GetAttachment();
    std::shared_ptr<DeviceProfile> getProfile(int32_t virtualSlot);
    int8_t& getLeftStickX(int32_t virtualSlot);
    int8_t& getLeftStickY(int32_t virtualSlot);
    int8_t& getRightStickX(int32_t virtualSlot);
    int8_t& getRightStickY(int32_t virtualSlot);
    int32_t& getPressedButtons(int32_t virtualSlot);
    float& getGyroX(int32_t virtualSlot);
    float& getGyroY(int32_t virtualSlot);
    bool IsRumbling();
    std::string GetGuid();

  protected:
    std::shared_ptr<ControllerAttachment> mAttachment;
    std::string mGuid;
    bool mIsRumbling;

    void LoadBinding();
    int8_t ReadStick(int32_t virtualSlot, Stick stick, Axis axis);
    void ProcessStick(int8_t& x, int8_t& y, float deadzoneX, float deadzoneY, int32_t notchProxmityThreshold);

  private:
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

    double GetClosestNotch(double angle, double approximationThreshold);
};
} // namespace Ship
