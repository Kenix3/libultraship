#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
#include <ImGui/imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include "controller/controldevice/controller/Controller.h"

namespace LUS {

class ControllerReorderingWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~ControllerReorderingWindow();
    int32_t GetSDLIndex();

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;

  private:
    int32_t mSDLIndex;
};
} // namespace LUS
