#include "controller/controldevice/controller/mapping/ControllerRumbleMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLRumbleMapping final : public ControllerRumbleMapping, public SDLMapping {
  public:
    SDLRumbleMapping(uint8_t portIndex, uint8_t lowFrequencyIntensityPercentage,
                     uint8_t highFrequencyIntensityPercentage, int32_t sdlControllerIndex);

    void StartRumble() override;
    void StopRumble() override;
    void SetLowFrequencyIntensity(uint8_t intensityPercentage) override;
    void SetHighFrequencyIntensity(uint8_t intensityPercentage) override;

    std::string GetRumbleMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

    std::string GetPhysicalDeviceName() override;

  private:
    uint16_t mLowFrequencyIntensity;
    uint16_t mHighFrequencyIntensity;
};
} // namespace LUS