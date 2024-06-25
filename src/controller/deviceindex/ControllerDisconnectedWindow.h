#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include "controller/controldevice/controller/Controller.h"
#include "ShipDeviceIndex.h"

namespace Ship {

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
    void DrawKnownControllerDisconnected();
    int32_t GetSDLIndexFromSDLInput();

    uint8_t mPortIndexOfDisconnectedController;
};
} // namespace Ship
