#pragma once

#include <string>

namespace Ship {
class GuiElement {
  public:
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
    virtual void DrawElement() = 0;

  protected:
    virtual void InitElement() = 0;
    virtual void UpdateElement() = 0;

    virtual void SetVisiblity(bool visible);
    bool mIsVisible;

  private:
    bool mIsInitialized;
};

} // namespace Ship
