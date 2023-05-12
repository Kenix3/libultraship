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
    if (mIsInitialized) {
        return;
    }

    InitElement();
    mIsInitialized = true;
}

void GuiElement::Draw() {
    if (!mIsVisible) {
        return;
    }

    DrawElement();
}

void GuiElement::Update() {
    UpdateElement();
}

void GuiElement::SetConsoleVariable() {
    if (mConsoleVariable.empty()) {
        return;
    }

    CVarSetInteger(mConsoleVariable.c_str(), mIsVisible);
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