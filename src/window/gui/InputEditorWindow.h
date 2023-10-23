#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
#include <ImGui/imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
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
    void DrawStickDirectionLine(const char* axisDirectionName, uint8_t port, uint8_t stick, Direction direction,
                                ImVec4 color);
    void DrawButtonLine(const char* buttonName, uint8_t port, uint16_t bitmask, ImVec4 color);
    void DrawButtonLineEditMappingButton(uint8_t port, uint16_t bitmask, std::string id);
    void DrawButtonLineAddMappingButton(uint8_t port, uint16_t bitmask);

    void DrawStickDirectionLineEditMappingButton(uint8_t port, uint8_t stick, Direction direction, std::string id);
    void DrawStickDirectionLineAddMappingButton(uint8_t port, uint8_t stick, Direction direction);
    void DrawStickSection(uint8_t port, uint8_t stick, int32_t id, ImVec4 color);

    void DrawRumbleSection(uint8_t port);
    void DrawRemoveRumbleMappingButton(uint8_t port, std::string id);
    void DrawAddRumbleMappingButton(uint8_t port);

    void DrawLEDSection(uint8_t port);
    void DrawRemoveLEDMappingButton(uint8_t port, std::string id);
    void DrawAddLEDMappingButton(uint8_t port);

    void DrawGyroSection(uint8_t port);
    void DrawRemoveGyroMappingButton(uint8_t port, std::string id);
    void DrawAddGyroMappingButton(uint8_t port);

    int32_t mGameInputBlockTimer;

    // mBitmaskToMappingIds[port][bitmask] = { id0, id1, ... }
    std::unordered_map<uint8_t, std::unordered_map<uint16_t, std::vector<std::string>>> mBitmaskToMappingIds;

    // mStickDirectionToMappingIds[port][stick][direction] = { id0, id1, ... }
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, std::unordered_map<Direction, std::vector<std::string>>>>
        mStickDirectionToMappingIds;

    void UpdateBitmaskToMappingIds(uint8_t port);
    void UpdateStickDirectionToMappingIds(uint8_t port);

    void GetButtonColorsForLUSDeviceIndex(LUSDeviceIndex lusIndex, ImVec4& buttonColor, ImVec4& buttonHoveredColor);
    void DrawPortTab(uint8_t portIndex);
    void DrawDevicesTab();
    std::set<uint16_t> mButtonsBitmasks;
    std::set<uint16_t> mDpadBitmasks;
    void DrawButtonDeviceIcons(uint8_t portIndex, std::set<uint16_t> bitmasks);
    void DrawAnalogStickDeviceIcons(uint8_t portIndex, LUS::Stick stick);
    void DrawRumbleDeviceIcons(uint8_t portIndex);
    void DrawGyroDeviceIcons(uint8_t portIndex);
    void DrawLEDDeviceIcons(uint8_t portIndex);
};
} // namespace LUS
