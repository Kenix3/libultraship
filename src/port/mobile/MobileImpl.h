#pragma once

#include <cstdint>
#include <string>

#include <ImGui/imgui.h>

namespace LUS {

class Mobile {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
};
}; // namespace LUS
