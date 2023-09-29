#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
#include <ImGui/imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include "controller/controldevice/controller/Controller.h"

namespace LUS {

class InputEditorWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~InputEditorWindow();

    void DrawButton(const char* label, int32_t n64Btn, int32_t currentPort, int32_t* btnReading);

    void DrawInputChip(const char* buttonName, ImVec4 color);
    void DrawAnalogPreview(const char* label, ImVec2 stick, float deadzone = 0);
    void DrawControllerSchema();

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;

  private:
    void DrawStickSection(int32_t* notchProximityThreshold, uint8_t port, uint8_t stick, int32_t id, ImVec4 color);
    void DrawStickDirectionLine(const char* axisDirectionName, uint8_t port, uint8_t stick, Direction direction,
                                ImVec4 color);
    void DrawButtonLine(const char* buttonName, uint8_t port, uint16_t bitmask, ImVec4 color);
    void DrawButtonLineEditMappingButton(uint8_t port, uint16_t bitmask, std::string id);
    void DrawButtonLineAddMappingButton(uint8_t port, uint16_t bitmask);

    void DrawStickDirectionLineEditMappingButton(uint8_t port, uint8_t stick, Direction direction, std::string id);
    void DrawStickDirectionLineAddMappingButton(uint8_t port, uint8_t stick, Direction direction);

    int32_t mBtnReading;
    int32_t mGameInputBlockTimer;

    // mBitmaskToMappingIds[port][bitmask] = { id, id }
    std::unordered_map<uint8_t, std::unordered_map<uint16_t, std::vector<std::string>>> mBitmaskToMappingIds;

    // mStickDirectionToMappingIds[port][stick][direction] = { id, id }
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, std::unordered_map<Direction, std::vector<std::string>>>>
        mStickDirectionToMappingIds;

    void UpdateBitmaskToMappingIds(uint8_t port);
    void UpdateStickDirectionToMappingIds(uint8_t port);
};
} // namespace LUS
