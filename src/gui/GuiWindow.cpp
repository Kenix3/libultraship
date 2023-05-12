#include "gui/GuiWindow.h"

namespace LUS {
GuiWindow::GuiWindow(const std::string& name, bool isOpen) : mName(name), mIsOpen(isOpen) {
}

GuiWindow::GuiWindow(const std::string& name) : GuiWindow(name, false) {
}

void GuiWindow::Open() {
    mIsOpen = true;
}

void GuiWindow::Close() {
    mIsOpen = false;
}

bool GuiWindow::IsOpen() {
    return mIsOpen;
}

std::string GuiWindow::GetName() {
    return mName;
}
} // namespace LUS
