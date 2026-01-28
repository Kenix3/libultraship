#pragma once

#include <cstdint>
#include <string>

#include <imgui.h>

namespace Ship {

class Mobile {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
    static bool IsUsingTouchscreenControls();
    static void EnableTouchArea();
    static void DisableTouchArea();
    static void SetMenuOpen(bool open);
    static float GetCameraYaw();
    static float GetCameraPitch();
};
}; // namespace Ship
