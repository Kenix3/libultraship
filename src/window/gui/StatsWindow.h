#pragma once

#include "window/gui/GuiWindow.h"

namespace ShipDK {
class StatsWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~StatsWindow();

  private:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};
} // namespace ShipDK