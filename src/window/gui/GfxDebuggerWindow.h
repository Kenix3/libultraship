#pragma once

#include "window/gui/GuiWindow.h"
#include <vector>

union F3DGfx;

namespace LUS {

class GfxDebuggerWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~GfxDebuggerWindow();

  protected:
    void InitElement() override;
    void UpdateElement() override;
    void DrawElement() override;

  private:
    void DrawDisasNode(const F3DGfx* cmd, std::vector<const F3DGfx*>& gfx_path) const;
    void DrawDisas();

  private:
    std::vector<const F3DGfx*> mLastBreakPoint = {};
};

} // namespace LUS
