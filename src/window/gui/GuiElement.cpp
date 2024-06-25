#include "GuiElement.h"

#include "libultraship/libultraship.h"

namespace Ship {
GuiElement::GuiElement(bool isVisible) : mIsInitialized(false), mIsVisible(isVisible) {
}

GuiElement::GuiElement() : GuiElement(false) {
}

GuiElement::~GuiElement() {
}

void GuiElement::Init() {
    if (IsInitialized()) {
        return;
    }

    InitElement();
    mIsInitialized = true;
}

void GuiElement::Update() {
    UpdateElement();
}

void GuiElement::SetVisiblity(bool visible) {
    mIsVisible = visible;
}

void GuiElement::Show() {
    SetVisiblity(true);
}

void GuiElement::Hide() {
    SetVisiblity(false);
}

void GuiElement::ToggleVisibility() {
    SetVisiblity(!IsVisible());
}

bool GuiElement::IsVisible() {
    return mIsVisible;
}

bool GuiElement::IsInitialized() {
    return mIsInitialized;
}
} // namespace Ship
