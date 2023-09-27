#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace LUS {

class ControllerStick {
  public:
    ControllerStick(uint8_t portIndex, Stick stick);
    ~ControllerStick();

    void ReloadAllMappingsFromConfig();
    void ResetToDefaultMappings(int32_t sdlControllerIndex);

    void ClearAllMappings();
    void UpdatePad(int8_t& x, int8_t& y);
    std::shared_ptr<ControllerAxisDirectionMapping> GetAxisDirectionMappingById(Direction direction, std::string id);
    std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>> GetAllAxisDirectionMappingByDirection(Direction direction);
    void AddAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping);
    void ClearAxisDirectionMapping(Direction direction, std::string id);
    void ClearAxisDirectionMapping(Direction direction, std::shared_ptr<ControllerAxisDirectionMapping> mapping);
    void SaveAxisDirectionMappingIdsToConfig();
    bool AddOrEditAxisDirectionMappingFromRawPress(Direction direction, std::string id);
    void Process(int8_t& x, int8_t& y);

    void ProcessKeyboardAllKeysUp();
    bool ProcessKeyboardKeyUp(int32_t scancode);
    bool ProcessKeyboardKeyDown(int32_t scancode);

  private:
    double GetClosestNotch(double angle, double approximationThreshold);
    void LoadAxisDirectionMappingFromConfig(std::string id);
    float GetAxisDirectionValue(Direction direction);

    uint8_t mPortIndex;
    Stick mStick;

    // TODO: handle deadzones separately for X and Y?
    float mDeadzone;
    int32_t mNotchProxmityThreshold;

    std::unordered_map<Direction, std::unordered_map<std::string, std::shared_ptr<ControllerAxisDirectionMapping>>> mAxisDirectionMappings;
};
} // namespace LUS
