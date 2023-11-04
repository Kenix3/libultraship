#pragma once

#include <cstdint>
#include <string>

#include <ImGui/imgui.h>

namespace LUS {

class Android {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
};
}; // namespace LUS
