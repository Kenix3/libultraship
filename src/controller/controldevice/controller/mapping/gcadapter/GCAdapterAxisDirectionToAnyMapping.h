#pragma once

#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

namespace Ship {
class GCAdapterAxisDirectionToAnyMapping : virtual public ControllerInputMapping {
  public:
    GCAdapterAxisDirectionToAnyMapping(uint8_t gcAxis, int32_t axisDirection);
    ~GCAdapterAxisDirectionToAnyMapping();

    std::string GetPhysicalDeviceName() override;
    std::string GetPhysicalInputName() override;

  protected:
    uint8_t mGcAxis;
    int32_t mAxisDirection;
};
} // namespace Ship