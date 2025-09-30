#pragma once

#include "ship/window/gui/GuiWindow.h"

namespace Ship {

class SDLAddRemoveDeviceEventHandler : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~SDLAddRemoveDeviceEventHandler();

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};
} // namespace Ship
