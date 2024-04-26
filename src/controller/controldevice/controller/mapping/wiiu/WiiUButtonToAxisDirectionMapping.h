#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "WiiUButtonToAnyMapping.h"

namespace ShipDK {
class WiiUButtonToAxisDirectionMapping final : public ControllerAxisDirectionMapping, public WiiUButtonToAnyMapping {
  public:
    WiiUButtonToAxisDirectionMapping(ShipDKDeviceIndex shipDKDeviceIndex, uint8_t portIndex, Stick stick,
                                     Direction direction, bool isNunchuk, bool isClassic,
                                     uint32_t wiiuControllerButton);

    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
};
} // namespace ShipDK
#endif
