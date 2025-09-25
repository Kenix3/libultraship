#pragma once

#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "controller/controldevice/controller/mapping/IControllerAxisRemapper.h"
#include "GCAdapterAxisDirectionToAnyMapping.h"
#include "controller/physicaldevice/gc/GCAdapter.h"

namespace Ship {
class GCAdapterAxisDirectionToAxisDirectionMapping : public ControllerAxisDirectionMapping,
                                                     public GCAdapterAxisDirectionToAnyMapping,
                                                     public IControllerAxisRemapper {
  public:
    GCAdapterAxisDirectionToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex, Direction direction,
                                                 uint8_t gcAxis, int32_t axisDirection);

    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
    int8_t GetMappingType() override;
    void RemapStick(double& x, double& y) override;
};
} // namespace Ship