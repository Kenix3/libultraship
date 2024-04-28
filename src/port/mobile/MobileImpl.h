#pragma once

#include <cstdint>
#include <string>

#include <ImGui/imgui.h>

namespace ShipDK {

class Mobile {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
};
}; // namespace ShipDK
