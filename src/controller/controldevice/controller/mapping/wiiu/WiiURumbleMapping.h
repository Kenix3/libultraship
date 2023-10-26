#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include "WiiUMapping.h"

namespace LUS {
class WiiURumbleMapping final : public ControllerRumbleMapping, public WiiUMapping {
  public:
    WiiURumbleMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint8_t lowFrequencyIntensityPercentage,
                      uint8_t highFrequencyIntensityPercentage);

    void StartRumble() override;
    void StopRumble() override;
    void SetLowFrequencyIntensity(uint8_t intensityPercentage) override;
    void SetHighFrequencyIntensity(uint8_t intensityPercentage) override;

    std::string GetRumbleMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  private:
    void SetIntensity(uint8_t intensityPercentage);
    uint8_t mRumblePattern[15];
};
} // namespace LUS
#endif
