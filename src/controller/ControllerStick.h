#pragma once

#include "AxisDirectionMapping.h"
#include <cstdint>
#include <memory>

namespace LUS {
enum Direction { LEFT, RIGHT, UP, DOWN };

class ControllerStick {
  public:
    ControllerStick();
    ~ControllerStick();

    void ReloadAllMappings();

    void ClearAllMappings();
    void UpdatePad(int8_t& x, int8_t& y);
    void UpdateAxisDirectionMapping(Direction direction, std::shared_ptr<AxisDirectionMapping> mapping);

  private:
    void Process(int8_t& x, int8_t& y);
    double GetClosestNotch(double angle, double approximationThreshold);

    // TODO: handle deadzones separately for X and Y?
    float mDeadzone;
    int32_t mNotchProxmityThreshold;

    std::shared_ptr<AxisDirectionMapping> mUpMapping;
    std::shared_ptr<AxisDirectionMapping> mDownMapping;
    std::shared_ptr<AxisDirectionMapping> mLeftMapping;
    std::shared_ptr<AxisDirectionMapping> mRightMapping;
};
} // namespace LUS
