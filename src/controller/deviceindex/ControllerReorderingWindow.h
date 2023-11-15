#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
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

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;

  private:
#ifdef __WIIU__
    std::vector<int32_t> GetConnectedWiiUDevices();
    int32_t GetWiiUDeviceFromWiiUInput();
#else
    int32_t GetSDLIndexFromSDLInput();
#endif
    std::vector<int32_t> mDeviceIndices;
    uint8_t mCurrentPortNumber;
};
} // namespace LUS
