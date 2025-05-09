#pragma once

#include "stdint.h"
#include "window/gui/GuiWindow.h"
#include <imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include "controller/controldevice/controller/Controller.h"

namespace Ship {

class InputEditorWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~InputEditorWindow();

    void DrawInputChip(const char* buttonName, ImVec4 color);
    void DrawAnalogPreview(const char* label, ImVec2 stick, float deadzone = 0, bool gyro = false);
    bool TestingRumble();

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;

  private:
    void DrawStickDirectionLine(const char* axisDirectionName, uint8_t port, uint8_t stick, Direction direction,
                                ImVec4 color);
    void DrawButtonLine(const char* buttonName, uint8_t port, CONTROLLERBUTTONS_T bitmask, ImVec4 color);
    void DrawButtonLineEditMappingButton(uint8_t port, CONTROLLERBUTTONS_T bitmask, std::string id);
    void DrawButtonLineAddMappingButton(uint8_t port, CONTROLLERBUTTONS_T bitmask);

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
    int32_t mMappingInputBlockTimer;
    int32_t mRumbleTimer;
    std::shared_ptr<ControllerRumbleMapping> mRumbleMappingToTest;

    // mBitmaskToMappingIds[port][bitmask] = { id0, id1, ... }
    std::unordered_map<uint8_t, std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::string>>> mBitmaskToMappingIds;

    // mStickDirectionToMappingIds[port][stick][direction] = { id0, id1, ... }
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, std::unordered_map<Direction, std::vector<std::string>>>>
        mStickDirectionToMappingIds;

    void UpdateBitmaskToMappingIds(uint8_t port);
    void UpdateStickDirectionToMappingIds(uint8_t port);

    void GetButtonColorsForPhysicalDeviceType(PhysicalDeviceType physicalDeviceType, ImVec4& buttonColor,
                                              ImVec4& buttonHoveredColor);
    void DrawPortTab(uint8_t portIndex);
    std::set<CONTROLLERBUTTONS_T> mButtonsBitmasks;
    std::set<CONTROLLERBUTTONS_T> mDpadBitmasks;
    bool mInputEditorPopupOpen;
    void DrawSetDefaultsButton(uint8_t portIndex);
    void DrawClearAllButton(uint8_t portIndex);

    void DrawDeviceToggles(uint8_t portIndex);
    void OffsetMappingPopup();
};
} // namespace Ship
