#pragma once

#include "menu/GuiWindow.h"

namespace LUS {
class StatsWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void Init() override;
    void Draw() override;
    void Update() override;
};
} // namespace LUS