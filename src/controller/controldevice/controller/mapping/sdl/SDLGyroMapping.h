#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLGyroMapping final : public ControllerGyroMapping, public SDLMapping {
  public:
    SDLGyroMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, float sensitivity, float neutralPitch,
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
