#pragma once

#include <string>
#include "window/gui/GuiElement.h"

namespace LUS {
class GuiWindow : public GuiElement {
  public:
    GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name);
    GuiWindow(const std::string& consoleVariable, const std::string& name);

    std::string GetName();

  protected:
  private:
    std::string mName;
};
} // namespace LUS
