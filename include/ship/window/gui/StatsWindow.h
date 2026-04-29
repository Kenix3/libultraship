#pragma once

#include "ship/window/gui/GuiWindow.h"

namespace Ship {
/**
 * @brief An ImGui window that displays runtime performance statistics.
 *
 * StatsWindow shows per-frame metrics such as FPS, frame time, memory usage,
 * and archive load statistics. It inherits the standard GuiWindow lifecycle
 * and is accessible via Gui::GetGuiWindow("Stats").
 */
class StatsWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    /** @brief Virtual destructor. */
    virtual ~StatsWindow();

  protected:
    /** @brief Performs one-time setup (no additional initialisation required). */
    void InitElement() override;

    /** @brief Renders the stats panel contents via ImGui. */
    void DrawElement() override;

    /** @brief Updates cached counters and timing values before each draw. */
    void UpdateElement() override;
};
} // namespace Ship