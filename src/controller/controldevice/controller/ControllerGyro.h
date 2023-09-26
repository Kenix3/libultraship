#pragma once

#include <cstdint>
#include <memory>

#include "controller/controldevice/controller/mapping/ControllerGyroAxisMapping.h"

namespace LUS {
class ControllerGyro {
  public:
    ControllerGyro();
    ~ControllerGyro();

    std::shared_ptr<ControllerGyroAxisMapping> GetGyroXMapping();
    std::shared_ptr<ControllerGyroAxisMapping> GetGyroYMapping();

    void UpdatePad(float& x, float& y);

  private:
    std::shared_ptr<ControllerGyroAxisMapping> mGyroXMapping;
    std::shared_ptr<ControllerGyroAxisMapping> mGyroYMapping;
};
} // namespace LUS
