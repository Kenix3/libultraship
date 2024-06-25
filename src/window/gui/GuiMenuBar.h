#pragma once

#include <string>
#include "window/gui/GuiElement.h"

namespace Ship {
class GuiMenuBar : public GuiElement {
  public:
    using GuiElement::GuiElement;
    void GuiMenuBar(const std::string& consoleVariable, bool isVisible);
    void Draw() override;
};
} // namespace Ship
