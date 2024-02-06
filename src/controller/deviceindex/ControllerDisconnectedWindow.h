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
#include "LUSDeviceIndex.h"

namespace LUS {

class ControllerDisconnectedWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~ControllerDisconnectedWindow();
    void SetPortIndexOfDisconnectedController(uint8_t portIndex);

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;

  private:
    void DrawUnknownOrMultipleControllersDisconnected();
#ifdef __WIIU__
    int32_t GetWiiUDeviceFromWiiUInput();
    bool AnyWiiUDevicesAreConnected();
#else
    void DrawKnownControllerDisconnected();
    int32_t GetSDLIndexFromSDLInput();
#endif

    uint8_t mPortIndexOfDisconnectedController;
};
} // namespace LUS
