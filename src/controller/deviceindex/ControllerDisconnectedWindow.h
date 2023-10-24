#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
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
    void DrawKnownControllerDisconnected();
    void DrawUnknownOrMultipleControllersDisconnected();
    int32_t GetSDLIndexFromSDLInput();
    uint8_t mPortIndexOfDisconnectedController;
};
} // namespace LUS
