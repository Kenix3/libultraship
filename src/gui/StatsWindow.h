#pragma once

#include "gui/GuiWindow.h"

namespace LUS {
class StatsWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;

  private:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};
} // namespace LUS