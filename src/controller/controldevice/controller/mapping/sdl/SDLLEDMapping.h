#include "controller/controldevice/controller/mapping/ControllerLEDMapping.h"
#include "SDLMapping.h"

namespace Ship {
class SDLLEDMapping final : public ControllerLEDMapping, public SDLMapping {
  public:
    SDLLEDMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, uint8_t colorSource, Color_RGB8 savedColor);

    void SetLEDColor(Color_RGB8 color) override;

    std::string GetLEDMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;
};
} // namespace Ship
