#pragma once

#include <string>

namespace Ship {
class GuiElement {
  public:
    GuiElement(const std::string& visibilityConsoleVariable, bool isVisible);
    GuiElement(const std::string& visibilityConsoleVariable);
    GuiElement(bool isVisible);
    GuiElement();
    ~GuiElement();

    void Init();
    virtual void Draw() = 0;
    void Update();

    void Show();
    void Hide();
    void ToggleVisibility();
    bool IsVisible();
    bool IsInitialized();

  protected:
    virtual void InitElement() = 0;
    virtual void DrawElement() = 0;
    virtual void UpdateElement() = 0;

    void SetVisiblity(bool visible);
    bool mIsVisible;

  private:
    bool mIsInitialized;
};

} // namespace Ship
