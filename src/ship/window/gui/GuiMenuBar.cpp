#include "ship/window/gui/GuiMenuBar.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

namespace Ship {
GuiMenuBar::GuiMenuBar(const std::string& visibilityConsoleVariable, bool isVisible)
    : GuiElement(visibilityConsoleVariable, isVisible), mVisibilityConsoleVariable(visibilityConsoleVariable) {
}

GuiMenuBar::GuiMenuBar(const std::string& visibilityConsoleVariable) : GuiMenuBar(visibilityConsoleVariable, false) {
}

void GuiMenuBar::OnInit(const nlohmann::json& initArgs) {
    GuiElement::OnInit(initArgs);
    mConsoleVariable = Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>();
    mWindow = Context::GetInstance()->GetChildren().GetFirst<Window>();
    if (!mVisibilityConsoleVariable.empty()) {
        mIsVisible = mConsoleVariable->GetInteger(mVisibilityConsoleVariable.c_str(), mIsVisible);
        SyncVisibilityConsoleVariable();
    }
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

    bool shouldSave = mConsoleVariable->GetInteger(mVisibilityConsoleVariable.c_str(), 0) != IsVisible();

    if (IsVisible()) {
        mConsoleVariable->SetInteger(mVisibilityConsoleVariable.c_str(), IsVisible());
    } else {
        mConsoleVariable->ClearVariable(mVisibilityConsoleVariable.c_str());
    }

    if (shouldSave) {
        mWindow->GetGui()->SaveConsoleVariablesNextFrame();
    }
}

void GuiMenuBar::SetVisibility(bool visible) {
    mIsVisible = visible;
    SyncVisibilityConsoleVariable();
}
} // namespace Ship
