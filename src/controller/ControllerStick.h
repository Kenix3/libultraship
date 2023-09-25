#pragma once

#include "AxisDirectionMapping.h"
#include <cstdint>
#include <memory>

namespace LUS {
enum Stick { LEFT_STICK, RIGHT_STICK };
enum Direction { LEFT, RIGHT, UP, DOWN };

class ControllerStick {
  public:
    ControllerStick(Stick stick);
    ~ControllerStick();

    void ReloadAllMappingsFromConfig(uint8_t port);
    void ResetToDefaultMappings(uint8_t port, int32_t sdlControllerIndex);

    void ClearAllMappings();
    void UpdatePad(int8_t& x, int8_t& y);
    std::shared_ptr<AxisDirectionMapping> GetAxisDirectionMappingByDirection(Direction direction);
    void UpdateAxisDirectionMapping(Direction direction, std::shared_ptr<AxisDirectionMapping> mapping);
    void SaveToConfig(uint8_t port);

  private:
    void Process(int8_t& x, int8_t& y);
    double GetClosestNotch(double angle, double approximationThreshold);
    void LoadAxisDirectionMappingFromConfig(std::string uuid, Direction direction);

    // TODO: handle deadzones separately for X and Y?
    float mDeadzone;
    int32_t mNotchProxmityThreshold;
    Stick mStick;

    std::shared_ptr<AxisDirectionMapping> mUpMapping;
    std::shared_ptr<AxisDirectionMapping> mDownMapping;
    std::shared_ptr<AxisDirectionMapping> mLeftMapping;
    std::shared_ptr<AxisDirectionMapping> mRightMapping;
};
} // namespace LUS
