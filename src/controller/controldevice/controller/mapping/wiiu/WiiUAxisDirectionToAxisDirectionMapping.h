#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "WiiUMapping.h"

namespace LUS {
class WiiUAxisDirectionToAxisDirectionMapping final : public ControllerAxisDirectionMapping,
                                                     public WiiUMapping {
  public:
    WiiUAxisDirectionToAxisDirectionMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, Stick stick,
                                           Direction direction, int32_t wiiuControllerAxis, int32_t axisDirection);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

  private:
    int32_t mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
#endif
