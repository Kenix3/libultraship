#include "controller/controldevice/controller/mapping/ControllerGyroMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLGyroMapping final : public ControllerGyroMapping, public SDLMapping {
  public:
    SDLGyroMapping(uint8_t portIndex, int32_t sdlControllerIndex);
    void UpdatePad(float& x, float& y) override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
    std::string GetGyroMappingId() override;

    std::string GetPhysicalDeviceName() override;

  private:
    uint8_t mBlarg;
};
} // namespace LUS
