#pragma once

#include "ship/window/gui/GuiWindow.h"
#include <imgui.h>

namespace Ship {

class Events;

/**
 * @brief An ImGui window for inspecting and debugging engine events at runtime.
 *
 * EventDebuggerWindow displays a live view of events flowing through the
 * EventSystem, allowing developers to monitor and diagnose event dispatch.
 * It inherits the standard GuiWindow lifecycle.
 */
class EventDebuggerWindow final : public GuiWindow {
  public:
    /**
     * @brief Constructs an EventDebuggerWindow with constructor-injected dependencies.
     * @param consoleVariable ConsoleVariable dependency (threaded from GuiWindow).
     * @param window          Window dependency (threaded from GuiWindow).
     * @param events          Events subsystem for live event inspection.
     * @param visibilityCvar  CVar name for window visibility.
     * @param name            Window title.
     */
    EventDebuggerWindow(std::shared_ptr<ConsoleVariable> consoleVariable, std::shared_ptr<Window> window,
                        std::shared_ptr<Events> events, const std::string& visibilityCvar, const std::string& name);
    using GuiWindow::GuiWindow;

    /** @brief Performs one-time initialization of the event debugger window. */
    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;

    /** @brief Renders the event debugger panel contents via ImGui. */
    void DrawElement() override;

    /** @brief No-op update; event data is read directly during draw. */
    void UpdateElement() override{};

  private:
    std::shared_ptr<Events> mEvents;
};

} // namespace Ship