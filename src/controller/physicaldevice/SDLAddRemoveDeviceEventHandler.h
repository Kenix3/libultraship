#pragma once

#include "window/gui/GuiWindow.h"

namespace Ship {

class SDLAddRemoveDeviceEventHandler : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~SDLAddRemoveDeviceEventHandler();

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};
} // namespace Ship
