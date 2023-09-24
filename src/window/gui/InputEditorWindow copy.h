// #pragma once

// #include "stdint.h"
// #include "window/gui/GuiWindow.h"
// #include <ImGui/imgui.h>

// namespace LUS {

// class InputEditorWindow : public GuiWindow {
//   public:
//     using GuiWindow::GuiWindow;
//     ~InputEditorWindow();

//     void DrawButton(const char* label, int32_t n64Btn, int32_t currentPort, int32_t* btnReading);
//     void DrawButtonLine(const char* buttonName, ImVec4 color);
//     void DrawAxisDirectionLine(const char* axisDirectionName, ImVec4 color);
//     void DrawInputChip(const char* buttonName, ImVec4 color);
//     void DrawControllerSelect(int32_t currentPort);
//     void DrawAnalogPreview(const char* label, ImVec2 stick, float deadzone = 0);
//     void DrawAnalogStickSection(int32_t* deadzone, int32_t* notchProximityThreshold, int32_t id, ImVec4 color);
//     void DrawControllerSchema();

//   protected:
//     void InitElement() override;
//     void DrawElement() override;
//     void UpdateElement() override;

//     int32_t mCurrentPort;

//   private:
//     int32_t mBtnReading;
//     int32_t mGameInputBlockTimer;
//     const int32_t mGameInputBlockId = -1;
// };
// } // namespace LUS
