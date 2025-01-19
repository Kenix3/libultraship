#pragma once

#include <vector>
#include <SDL2/SDL.h>

namespace Ship {

class ConnectedPhysicalDeviceManager {
  public:
    ConnectedPhysicalDeviceManager();
    ~ConnectedPhysicalDeviceManager();

    std::vector<SDL_GameController*> GetConnectedSDLGamepadsForPort(uint8_t portIndex);

  private:
    std::vector<SDL_GameController*> mConnectedSDLGamepads;
};
} // namespace Ship
