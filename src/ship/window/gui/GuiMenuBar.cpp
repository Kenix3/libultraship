#include "ship/window/gui/GuiMenuBar.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

namespace Ship {
GuiMenuBar::GuiMenuBar(const std::string& visibilityConsoleVariable, bool isVisible)
    : GuiElement(visibilityConsoleVariable, isVisible), mVisibilityConsoleVariable(visibilityConsoleVariable) {
    if (!mVisibilityConsoleVariable.empty()) {
        mIsVisible = Ship::Context::GetInstance()->GetChild<ConsoleVariable>()->GetInteger(mVisibilityConsoleVariable.c_str(),
                                                                                     mIsVisible);
        SyncVisibilityConsoleVariable();
    }
}

GuiMenuBar::GuiMenuBar(const std::string& visibilityConsoleVariable) : GuiMenuBar(visibilityConsoleVariable, false) {
}

void GuiMenuBar::Draw() {
    if (!IsVisible()) {
        return;
    }

    DrawElement();
    // Sync up the IsVisible flag if it was changed by ImGui
    SyncVisibilityConsoleVariable();
}

void GuiMenuBar::SyncVisibilityConsoleVariable() {
    if (mVisibilityConsoleVariable.empty()) {
        return;
    }

    bool shouldSave = Ship::Context::GetInstance()->GetChild<ConsoleVariable>()->GetInteger(
                          mVisibilityConsoleVariable.c_str(), 0) != IsVisible();

    if (IsVisible()) {
        Ship::Context::GetInstance()->GetChild<ConsoleVariable>()->SetInteger(mVisibilityConsoleVariable.c_str(),
                                                                        IsVisible());
    } else {
        Ship::Context::GetInstance()->GetChild<ConsoleVariable>()->ClearVariable(mVisibilityConsoleVariable.c_str());
    }

    if (shouldSave) {
        Context::GetInstance()->GetChild<Window>()->GetGui()->SaveConsoleVariablesNextFrame();
    }
}

void GuiMenuBar::SetVisibility(bool visible) {
    mIsVisible = visible;
    SyncVisibilityConsoleVariable();
}
} // namespace Ship
