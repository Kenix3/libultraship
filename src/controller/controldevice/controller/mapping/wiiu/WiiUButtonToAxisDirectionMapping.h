#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "WiiUButtonToAnyMapping.h"

namespace LUS {
class WiiUButtonToAxisDirectionMapping final : public ControllerAxisDirectionMapping, public WiiUButtonToAnyMapping {
  public:
    WiiUButtonToAxisDirectionMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, Stick stick, Direction direction,
                                     bool isNunchuk, bool isClassic, uint32_t wiiuControllerButton);

    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace LUS
#endif
