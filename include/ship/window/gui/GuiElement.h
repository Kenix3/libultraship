#pragma once

#include <string>
#include "ship/Component.h"

namespace Ship {
class GuiElement : public Component {
  public:
    GuiElement(const std::string& name, bool isVisible);
    GuiElement(const std::string& name);
    virtual ~GuiElement();

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

    virtual void SetVisibility(bool visible);
    bool mIsVisible;

  private:
    bool mIsInitialized;
};

} // namespace Ship
