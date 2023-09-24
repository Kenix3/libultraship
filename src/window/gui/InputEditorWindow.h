#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
#include <ImGui/imgui.h>

namespace LUS {

class InputEditorWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~InputEditorWindow();

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;

    int32_t mCurrentPort;

  private:
    int32_t mBtnReading;
    int32_t mGameInputBlockTimer;
    const int32_t mGameInputBlockId = -1;
};
} // namespace LUS
