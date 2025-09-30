#include "GuiElement.h"

#include "libultraship/libultraship.h"

namespace Ship {
GuiElement::GuiElement(bool isVisible) : mIsVisible(isVisible), mIsInitialized(false) {
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

void GuiElement::SetVisibility(bool visible) {
    mIsVisible = visible;
}

void GuiElement::Show() {
    SetVisibility(true);
}

void GuiElement::Hide() {
    SetVisibility(false);
}

void GuiElement::ToggleVisibility() {
    SetVisibility(!IsVisible());
}

bool GuiElement::IsVisible() {
    return mIsVisible;
}

bool GuiElement::IsInitialized() {
    return mIsInitialized;
}
} // namespace Ship
