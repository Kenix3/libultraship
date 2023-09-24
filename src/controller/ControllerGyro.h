#pragma once

#include <cstdint>
#include <memory>

#include "GyroAxisMapping.h"

namespace LUS {
class ControllerGyro {
  public:
    ControllerGyro();
    ~ControllerGyro();

    std::shared_ptr<GyroAxisMapping> GetGyroXMapping();
    std::shared_ptr<GyroAxisMapping> GetGyroYMapping();

    void UpdatePad(float& x, float& y);
  
  private:
    std::shared_ptr<GyroAxisMapping> mGyroXMapping;
    std::shared_ptr<GyroAxisMapping> mGyroYMapping;
};
} // namespace LUS
