#pragma once

#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "GCAdapterAxisDirectionToAnyMapping.h"
#include "controller/physicaldevice/gc/GCAdapter.h"

namespace Ship {
class GCAdapterAxisDirectionToButtonMapping final : public ControllerButtonMapping, public GCAdapterAxisDirectionToAnyMapping {
  public:
    GCAdapterAxisDirectionToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, uint8_t gcAxis,
                                          int32_t axisDirection);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    int8_t GetMappingType() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
    std::string GetPhysicalDeviceName() override;
    std::string GetPhysicalInputName() override;
};
} // namespace Ship
