#pragma once

#include "ship/window/gui/GuiWindow.h"
#include <imgui.h>

namespace Ship {

/**
 * @brief An ImGui window for inspecting and debugging engine events at runtime.
 *
 * EventDebuggerWindow displays a live view of events flowing through the
 * EventSystem, allowing developers to monitor and diagnose event dispatch.
 * It inherits the standard GuiWindow lifecycle.
 */
class EventDebuggerWindow final : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    /** @brief Performs one-time initialization of the event debugger window. */
    void InitElement() override;

    /** @brief Renders the event debugger panel contents via ImGui. */
    void DrawElement() override;

    /** @brief No-op update; event data is read directly during draw. */
    void UpdateElement() override{};
};

} // namespace Ship