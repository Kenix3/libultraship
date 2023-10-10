#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace LUS {

#define DEFAULT_STICK_DEADZONE_PERCENTAGE 20
#define DEFAULT_NOTCH_SNAP_ANGLE 0

class ControllerStick {
  public:
    ControllerStick(uint8_t portIndex, Stick stick);
    ~ControllerStick();

    void ReloadAllMappingsFromConfig();
    void AddDefaultMappings(LUSDeviceIndex lusIndex);

    void ClearAllMappings();
    void UpdatePad(int8_t& x, int8_t& y);
    std::shared_ptr<ControllerAxisDirectionMapping> GetAxisDirectionMappingById(Direction direction, std::string id);
    std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>
    GetAllAxisDirectionMappingByDirection(Direction direction);
    void AddAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping);
    void ClearAxisDirectionMapping(Direction direction, std::string id);
    void ClearAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping);
    void SaveAxisDirectionMappingIdsToConfig();
    bool AddOrEditAxisDirectionMappingFromRawPress(Direction direction, std::string id);
    void Process(int8_t& x, int8_t& y);

    void ResetDeadzoneToDefault();
    void SetDeadzone(uint8_t deadzonePercentage);
    uint8_t GetDeadzonePercentage();
    bool DeadzoneIsDefault();

    void ResetNotchSnapAngleToDefault();
    void SetNotchSnapAngle(uint8_t notchSnapAngle);
    uint8_t GetNotchSnapAngle();
    bool NotchSnapAngleIsDefault();

    bool ProcessKeyboardEvent(LUS::KbEventType eventType, LUS::KbScancode scancode);

    bool HasMappingsForLUSDeviceIndex(LUSDeviceIndex lusIndex);

  private:
    double GetClosestNotch(double angle, double approximationThreshold);
    void LoadAxisDirectionMappingFromConfig(std::string id);
    float GetAxisDirectionValue(Direction direction);

    uint8_t mPortIndex;
    Stick mStick;

    // TODO: handle deadzones separately for X and Y?
    uint8_t mDeadzonePercentage;
    float mDeadzone;
    uint8_t mNotchSnapAngle;

    std::unordered_map<Direction, std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>>
        mAxisDirectionMappings;

    bool mUseKeydownEventToCreateNewMapping;
    KbScancode mKeyboardScancodeForNewMapping;
};
} // namespace LUS
