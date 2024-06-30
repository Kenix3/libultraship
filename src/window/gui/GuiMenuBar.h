#pragma once

#include <string>
#include "window/gui/GuiElement.h"

namespace Ship {
class GuiMenuBar : public GuiElement {
  public:
    GuiMenuBar(const std::string& visibilityConsoleVariable, bool isVisible);
    GuiMenuBar(const std::string& visibilityConsoleVariable);
    void Draw() override;

  protected:
    void SetVisiblity(bool visible) override;

  private:
    void SyncVisibilityConsoleVariable();
    std::string mVisibilityConsoleVariable;
};
} // namespace Ship
