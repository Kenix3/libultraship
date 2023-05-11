#pragma once

#include "stdint.h"
#include "GuiWindow.h"
#include <ImGui/imgui.h>

namespace LUS {

class InputEditorWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void Init() override;
    void Draw() override;
    void Update() override;

    void DrawButton(const char* label, int32_t n64Btn, int32_t currentPort, int32_t* btnReading);
    void DrawControllerSelect(int32_t currentPort);
    void DrawVirtualStick(const char* label, ImVec2 stick);
    void DrawControllerSchema();

private:
    int32_t mCurrentPort;
    int32_t mBtnReading;
};
} // namespace LUS
