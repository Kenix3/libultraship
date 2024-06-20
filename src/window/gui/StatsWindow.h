#pragma once

#include "window/gui/GuiWindow.h"

namespace Ship {
class StatsWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~StatsWindow();
    void DrawContents() override;

  private:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};
} // namespace Ship