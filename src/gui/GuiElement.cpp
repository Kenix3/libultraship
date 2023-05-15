#include "GuiElement.h"

#include "libultraship/libultraship.h"

namespace LUS {
GuiElement::GuiElement(const std::string& visibilityConsoleVariable, bool isVisible)
    : mIsInitialized(false), mIsVisible(isVisible), mVisibilityConsoleVariable(visibilityConsoleVariable) {
    if (!mVisibilityConsoleVariable.empty()) {
        mIsVisible = CVarGetInteger(mVisibilityConsoleVariable.c_str(), mIsVisible);
        SyncVisibilityConsoleVariable();
    }
}

GuiElement::GuiElement(const std::string& visibilityConsoleVariable) : GuiElement(visibilityConsoleVariable, false) {
}

GuiElement::GuiElement(bool isVisible) : GuiElement("", isVisible) {
}

GuiElement::GuiElement() : GuiElement("", false) {
}

void GuiElement::Init() {
    if (IsInitialized()) {
        return;
    }

    InitElement();
    mIsInitialized = true;
}

void GuiElement::Draw() {
    if (!IsVisible()) {
        return;
    }

    DrawElement();
    // Sync up the IsVisible flag if it was changed by ImGui
    SyncVisibilityConsoleVariable();
}

void GuiElement::Update() {
    UpdateElement();
}

void GuiElement::SyncVisibilityConsoleVariable() {
    if (mVisibilityConsoleVariable.empty()) {
        return;
    }

    if (IsVisible()) {
        CVarSetInteger(mVisibilityConsoleVariable.c_str(), IsVisible());
    } else {
        CVarClear(mVisibilityConsoleVariable.c_str());
    }
}

void GuiElement::Show() {
    mIsVisible = true;
    SyncVisibilityConsoleVariable();
}

void GuiElement::Hide() {
    mIsVisible = false;
    SyncVisibilityConsoleVariable();
}

bool GuiElement::IsVisible() {
    return mIsVisible;
}

bool GuiElement::IsInitialized() {
    return mIsInitialized;
}
} // namespace LUS