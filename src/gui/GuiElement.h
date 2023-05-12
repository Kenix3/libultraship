#pragma once

#include <string>

namespace LUS {
class GuiElement {
  public:
    GuiElement(const std::string& consoleVariable, bool isVisible);
    GuiElement(const std::string& consoleVariable);
    GuiElement(bool isVisible);
    GuiElement();

    void Init();
    void Draw();
    void Update();

    void Show();
    void Hide();
    bool IsVisible();
    bool IsInitialized();

  protected:
    virtual void InitElement() = 0;
    virtual void DrawElement() = 0;
    virtual void UpdateElement() = 0;

    void SetConsoleVariable();

    bool mIsVisible;
    std::string mConsoleVariable;

  private:
    bool mIsInitialized;
};

} // namespace LUS
