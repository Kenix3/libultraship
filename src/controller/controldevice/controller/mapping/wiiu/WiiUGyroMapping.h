#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include "WiiUMapping.h"

namespace LUS {
class WiiUGyroMapping final : public ControllerGyroMapping, public WiiUMapping {
  public:
    WiiUGyroMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, float sensitivity, float neutralPitch,
                    float neutralYaw, float neutralRoll);
    void UpdatePad(float& x, float& y) override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
    void Recalibrate() override;
    std::string GetGyroMappingId() override;

    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  private:
    float mNeutralPitch;
    float mNeutralYaw;
    float mNeutralRoll;
};
} // namespace LUS
#endif
