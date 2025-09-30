#pragma once

#include "window/gui/GuiWindow.h"
#include <vector>
#include <memory>

namespace Fast {
union F3DGfx;
class Interpreter;
} // namespace Fast

namespace LUS {

class GfxDebuggerWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~GfxDebuggerWindow();

  protected:
    void InitElement() override;
    void UpdateElement() override;
    void DrawElement() override;

  private:
    void DrawDisasNode(const Fast::F3DGfx* cmd, std::vector<const Fast::F3DGfx*>& gfxPath, float parentPosY) const;
    void DrawDisas();

  private:
    std::vector<const Fast::F3DGfx*> mLastBreakPoint = {};
    std::weak_ptr<Fast::Interpreter> mInterpreter;
};

} // namespace LUS
