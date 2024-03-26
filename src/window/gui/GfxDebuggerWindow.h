#pragma once

#include "window/gui/GuiWindow.h"
#include "libultraship/libultra/gbi.h"
#include <vector>

namespace LUS {

class GfxDebuggerWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~GfxDebuggerWindow();

  protected:
    void InitElement() override;
    void UpdateElement() override;
    void DrawElement() override;

  private:
    void DrawDisasNode(const Gfx* cmd, std::vector<const Gfx*>& gfx_path) const;
    void DrawDisas();

  private:
    std::vector<const Gfx*> mLastBreakPoint = {};
};

} // namespace LUS
