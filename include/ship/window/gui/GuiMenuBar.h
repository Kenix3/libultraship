#pragma once

#include <string>
#include "ship/window/gui/GuiElement.h"

namespace Ship {
/**
 * @brief The application's main ImGui menu bar rendered at the top of the screen.
 *
 * GuiMenuBar wraps ImGui::BeginMainMenuBar() / ImGui::EndMainMenuBar() and integrates
 * with the CVar system so that its visibility can be toggled from the developer console
 * and persists across sessions.
 *
 * Subclass GuiMenuBar and implement DrawElement() to populate the menu bar with
 * ImGui::BeginMenu() / ImGui::MenuItem() calls. Register the instance with
 * Gui::SetMenuBar().
 */
class GuiMenuBar : public GuiElement {
  public:
    /**
     * @brief Constructs a GuiMenuBar with an explicit initial visibility.
     * @param visibilityConsoleVariable CVar name used to persist/read visibility.
     * @param isVisible                 Initial visibility state.
     */
    GuiMenuBar(const std::string& visibilityConsoleVariable, bool isVisible);

    /**
     * @brief Constructs a GuiMenuBar whose initial visibility is read from the CVar.
     * @param visibilityConsoleVariable CVar name used to persist/read visibility.
     */
    GuiMenuBar(const std::string& visibilityConsoleVariable);

    /**
     * @brief Calls ImGui::BeginMainMenuBar(), invokes DrawElement(), and ends the bar.
     *
     * Also syncs the visibility state back to the CVar if it changed.
     */
    void Draw() override;

  protected:
    /**
     * @brief Overrides SetVisibility to also write the new state to the backing CVar.
     * @param visible New visibility state.
     */
    void SetVisibility(bool visible) override;

  private:
    /** @brief Reads the CVar and updates mIsVisible accordingly. */
    void SyncVisibilityConsoleVariable();
    std::string mVisibilityConsoleVariable;
};
} // namespace Ship
