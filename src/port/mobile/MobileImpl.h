#pragma once

#include <cstdint>
#include <string>

#include <ImGui/imgui.h>

namespace Ship {

class Mobile {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
};
}; // namespace Ship
