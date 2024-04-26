#pragma once

#include <cstdint>
#include <string>

#include <ImGui/imgui.h>

namespace ShipDK {

class Android {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
};
}; // namespace ShipDK
