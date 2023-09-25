#include "controller/ButtonMapping.h"
#include "SDLMapping.h"

namespace LUS {
class SDLButtonToButtonMapping final : public ButtonMapping, public SDLMapping {
  public:
    SDLButtonToButtonMapping(uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    SDLButtonToButtonMapping(std::string uuid, uint16_t bitmask, int32_t sdlControllerIndex, int32_t sdlControllerButton);
    void UpdatePad(uint16_t& padButtons) override;
    uint8_t GetMappingType() override;
    std::string GetButtonName() override;
    void SaveToConfig() override;

  private:
    std::string GetPlaystationButtonName();
    std::string GetSwitchButtonName();
    std::string GetXboxButtonName();
    std::string GetGenericButtonName();

    SDL_GameControllerButton mControllerButton;
};
} // namespace LUS
