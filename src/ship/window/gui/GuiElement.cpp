#include "ship/window/gui/GuiElement.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"

namespace Ship {
GuiElement::GuiElement(const std::string& name, bool isVisible) : Component(name), mIsVisible(isVisible) {
}

GuiElement::GuiElement(const std::string& name) : GuiElement(name, false) {
}

GuiElement::~GuiElement() {
}

void GuiElement::OnInit(const nlohmann::json& /*initArgs*/) {
    InitElement();
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
} // namespace Ship
