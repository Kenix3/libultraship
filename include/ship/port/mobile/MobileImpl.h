#pragma once

#include <cstdint>
#include <string>

#include <imgui.h>

namespace Ship {

/**
 * @brief Platform abstraction layer for mobile-specific UI behaviour.
 *
 * Provides static helpers that bridge between the engine's ImGui layer and
 * mobile platform APIs (e.g. soft keyboard management on iOS/Android).
 */
class Mobile {
  public:
    /**
     * @brief Processes mobile-specific input events for ImGui.
     *
     * On mobile platforms this manages the lifecycle of the on-screen keyboard
     * based on whether ImGui currently wants text input.
     *
     * @param wantsTextInput true if an ImGui widget is requesting text input.
     */
    static void ImGuiProcessEvent(bool wantsTextInput);
};
}; // namespace Ship
