#pragma once

#include <string>
#include <imgui.h>
#include <imgui_internal.h>
#include "ship/window/gui/GuiElement.h"
#include <stdint.h>

namespace Ship {
/**
 * @brief A floating ImGui window managed by the Ship Gui layer.
 *
 * GuiWindow wraps a named ImGui::Begin / ImGui::End pair and integrates with the
 * CVar system: it stores its visibility state in a named console variable so that
 * the window can be shown/hidden from the developer console and the state persists
 * across sessions.
 *
 * Subclass GuiWindow and implement DrawElement() to add custom ImGui content.
 * Register the window with Gui::AddGuiWindow() so it participates in the draw loop.
 *
 * @code
 * class MyWindow : public Ship::GuiWindow {
 *   public:
 *     using GuiWindow::GuiWindow;
 *   protected:
 *     void InitElement() override { }
 *     void UpdateElement() override { }
 *     void DrawElement() override {
 *         ImGui::Text("Hello World");
 *     }
 * };
 * @endcode
 */
class GuiWindow : public GuiElement {
  public:
    /**
     * @brief Full constructor with explicit size and window flags.
     * @param consoleVariable  CVar name used to persist/read visibility (e.g. "gMyWindow").
     * @param isVisible        Initial visibility.
     * @param name             Window title shown in the ImGui title bar.
     * @param originalSize     Default size of the window on first open.
     * @param windowFlags      ImGui window flags (e.g. ImGuiWindowFlags_NoResize).
     */
    GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name, ImVec2 originalSize,
              uint32_t windowFlags);

    /**
     * @brief Constructor with explicit size, using default ImGui window flags.
     * @param consoleVariable CVar name for visibility.
     * @param isVisible       Initial visibility.
     * @param name            Window title.
     * @param originalSize    Default window size.
     */
    GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name, ImVec2 originalSize);

    /**
     * @brief Minimal constructor using default size and flags.
     * @param consoleVariable CVar name for visibility.
     * @param isVisible       Initial visibility.
     * @param name            Window title.
     */
    GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name);

    /**
     * @brief Constructor deriving initial visibility from the CVar value.
     * @param consoleVariable CVar name for visibility (its persisted value is used as initial state).
     * @param name            Window title.
     * @param originalSize    Default window size.
     * @param windowFlags     ImGui window flags.
     */
    GuiWindow(const std::string& consoleVariable, const std::string& name, ImVec2 originalSize, uint32_t windowFlags);

    /**
     * @brief Constructor deriving visibility from CVar, with default flags.
     * @param consoleVariable CVar name for visibility.
     * @param name            Window title.
     * @param originalSize    Default window size.
     */
    GuiWindow(const std::string& consoleVariable, const std::string& name, ImVec2 originalSize);

    /**
     * @brief Minimal constructor deriving visibility from CVar.
     * @param consoleVariable CVar name for visibility.
     * @param name            Window title.
     */
    GuiWindow(const std::string& consoleVariable, const std::string& name);

    /**
     * @brief Calls ImGui::Begin(), invokes DrawElement(), and calls ImGui::End().
     *
     * Also handles visibility changes and syncs the result back to the CVar.
     */
    void Draw() override;

  protected:
    /**
     * @brief Overrides SetVisibility to also write the new state to the backing CVar.
     * @param visible New visibility state.
     */
    void SetVisibility(bool visible) override;

    /**
     * @brief Begins a visually grouped labelled panel.
     * @param name Label text for the group panel.
     * @param size Requested size (use ImVec2(0,0) for auto-sizing).
     */
    void BeginGroupPanel(const char* name, const ImVec2& size);

    /**
     * @brief Ends the previously opened group panel, adding a bottom margin.
     * @param minHeight Minimum height for the panel; the actual height is at least this value.
     */
    void EndGroupPanel(float minHeight);

    /** @brief Reads the CVar and updates mIsVisible accordingly. */
    void SyncVisibilityConsoleVariable();

  private:
    std::string mVisibilityConsoleVariable;
    ImVector<ImRect> mGroupPanelLabelStack;
    ImVec2 mOriginalSize;
    uint32_t mWindowFlags;
};
} // namespace Ship
