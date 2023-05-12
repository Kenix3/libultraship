#pragma once

#include <string>

namespace LUS {
class GuiMenuBar {
  public:
    virtual void Init() = 0;
    virtual void Draw() = 0;
    virtual void Update() = 0;
};
} // namespace LUS
