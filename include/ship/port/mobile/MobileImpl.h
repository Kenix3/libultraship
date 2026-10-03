#pragma once

struct SDL_Window;

namespace Ship {

class Mobile {
  public:
    static void ImGuiProcessEvent(SDL_Window* window, bool wantsTextInput);
};
}; // namespace Ship
