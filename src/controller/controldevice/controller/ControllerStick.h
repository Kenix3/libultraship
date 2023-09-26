#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <cstdint>
#include <memory>

namespace LUS {
enum Stick { LEFT_STICK, RIGHT_STICK };
enum Direction { LEFT, RIGHT, UP, DOWN };

class ControllerStick {
  public:
    ControllerStick(uint8_t portIndex, Stick stick);
    ~ControllerStick();

    void ReloadAllMappingsFromConfig();
    void ResetToDefaultMappings(int32_t sdlControllerIndex);

    void ClearAllMappings();
    void UpdatePad(int8_t& x, int8_t& y);
    std::shared_ptr<ControllerAxisDirectionMapping> GetAxisDirectionMappingByDirection(Direction direction);
    void UpdateAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping);
    void SaveToConfig(uint8_t port);

  private:
    void Process(int8_t& x, int8_t& y);
    double GetClosestNotch(double angle, double approximationThreshold);
    void LoadAxisDirectionMappingFromConfig(std::string uuid, Direction direction);

    uint8_t mPortIndex;
    Stick mStick;

    // TODO: handle deadzones separately for X and Y?
    float mDeadzone;
    int32_t mNotchProxmityThreshold;
    
    std::shared_ptr<ControllerAxisDirectionMapping> mUpMapping;
    std::shared_ptr<ControllerAxisDirectionMapping> mDownMapping;
    std::shared_ptr<ControllerAxisDirectionMapping> mLeftMapping;
    std::shared_ptr<ControllerAxisDirectionMapping> mRightMapping;
};
} // namespace LUS
