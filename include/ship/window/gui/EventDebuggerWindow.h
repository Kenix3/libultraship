#pragma once

#include "ship/window/gui/GuiWindow.h"
#include <imgui.h>

namespace Ship {

class EventDebuggerWindow final : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override{};
};

} // namespace Ship