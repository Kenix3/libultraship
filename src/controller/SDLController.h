#pragma once
#include "Controller.h"
#include <SDL2/SDL.h>

namespace Ship {
class SDLController : public Controller {
  public:
    SDLController(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex);
    void ReadFromSource(int32_t portIndex) override;
    const std::string GetButtonName(int32_t portIndex, int32_t n64Button) override;
    void WriteToSource(int32_t portIndex, ControllerCallback* controller) override;
    bool Connected() const override;
    bool CanGyro() const override;
    bool CanRumble() const override;
    bool Open();
    void ClearRawPress() override;
    int32_t ReadRawPress() override;

  protected:
    inline static const char* AxisNames[] = { "Left Stick X", "Left Stick Y",  "Right Stick X", "Right Stick Y",
                                              "Left Trigger", "Right Trigger", "Start Button" };

    void CreateDefaultBinding(int32_t portIndex) override;

  private:
    SDL_GameController* mController;
    bool mSupportsGyro;
    void NormalizeStickAxis(SDL_GameControllerAxis axisX, SDL_GameControllerAxis axisY, int32_t portIndex);
    bool Close();
};
} // namespace Ship
