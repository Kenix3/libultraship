#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include "SDLMapping.h"

namespace Ship {
class SDLGyroMapping final : public ControllerGyroMapping {
  public:
    SDLGyroMapping(uint8_t portIndex, float sensitivity, float neutralPitch, float neutralYaw, float neutralRoll);
    void UpdatePad(float& x, float& y) override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
    void Recalibrate() override;
    std::string GetGyroMappingId() override;

    std::string GetPhysicalDeviceName() override;

  private:
    float mNeutralPitch;
    float mNeutralYaw;
    float mNeutralRoll;
};
} // namespace Ship
