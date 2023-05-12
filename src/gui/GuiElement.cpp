#include "GuiElement.h"

#include "libultraship/libultraship.h"

namespace LUS {
GuiElement::GuiElement(const std::string& consoleVariable, bool isVisible)
    : mIsInitialized(false), mIsVisible(isVisible), mConsoleVariable(consoleVariable) {
    if (!mConsoleVariable.empty()) {
        mIsVisible = CVarGetInteger(mConsoleVariable.c_str(), mIsVisible);
        SetConsoleVariable();
    }
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
    SetConsoleVariable();
}

void GuiElement::Update() {
    UpdateElement();
}

void GuiElement::SetConsoleVariable() {
    if (mConsoleVariable.empty()) {
        return;
    }

    if (IsVisible()) {
        CVarSetInteger(mConsoleVariable.c_str(), IsVisible());
    } else {
        CVarClear(mConsoleVariable.c_str());
    }
}

void GuiElement::Show() {
    mIsVisible = true;
    SetConsoleVariable();
}

void GuiElement::Hide() {
    mIsVisible = false;
    SetConsoleVariable();
}

bool GuiElement::IsVisible() {
    return mIsVisible;
}

bool GuiElement::IsInitialized() {
    return mIsInitialized;
}
} // namespace LUS