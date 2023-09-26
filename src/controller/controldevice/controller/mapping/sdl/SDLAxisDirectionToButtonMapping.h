#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLAxisDirectionToButtonMapping final : public ControllerButtonMapping, public SDLMapping {
  public:
    SDLAxisDirectionToButtonMapping(uint8_t portIndex, uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerAxis,
                                    int32_t axisDirection);
    void UpdatePad(uint16_t& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonName() override;
    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

  private:
    SDL_GameControllerAxis mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
