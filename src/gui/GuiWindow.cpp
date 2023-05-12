#include "gui/GuiWindow.h"

namespace LUS {
GuiWindow::GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name)
    : mName(name), GuiElement(consoleVariable, isVisible) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, const std::string& name)
    : GuiWindow(consoleVariable, false, name) {
}

std::string GuiWindow::GetName() {
    return mName;
}
} // namespace LUS
