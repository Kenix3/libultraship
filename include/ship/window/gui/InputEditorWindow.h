#pragma once

#include "stdint.h"
#include "ship/window/gui/GuiWindow.h"
#include <imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include "ship/controller/controldevice/controller/Controller.h"

namespace Ship {

/**
 * @brief An ImGui window for editing, binding, and testing controller mappings.
 *
 * InputEditorWindow provides a tab-based UI that allows the user to:
 * - View and remap button bindings for each controller port.
 * - View and remap analog stick axis bindings with sensitivity/dead-zone controls.
 * - Test, enable, and configure rumble mappings.
 * - Configure LED colour mappings.
 * - View gyroscope input and assign a gyro mapping.
 *
 * The window is added to the GUI by Context::Init() and is accessible via
 * Gui::GetGuiWindow("Input Editor").
 */
class InputEditorWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~InputEditorWindow();

    /**
     * @brief Renders a small coloured chip labelled with @p buttonName.
     *
     * Used to represent a single physical button within the editor layout.
     *
     * @param buttonName Human-readable button label.
     * @param color      Background colour of the chip.
     */
    void DrawInputChip(const char* buttonName, ImVec4 color);

    /**
     * @brief Renders a circular analog-stick preview widget.
     * @param label    Label displayed above the preview.
     * @param stick    Current stick position (X and Y in the range [-128, 127]).
     * @param deadzone Deadzone radius drawn as an inner circle (0 = no deadzone ring).
     * @param gyro     If true, renders the preview in "gyro" style.
     */
    void DrawAnalogPreview(const char* label, ImVec2 stick, float deadzone = 0, bool gyro = false);

    /**
     * @brief Returns true while the rumble test is actively playing.
     */
    bool TestingRumble();

  protected:
    /** @brief Registers button bitmasks and performs initial population of mapping tables. */
    void InitElement() override;

    /** @brief Renders the full port-tabbed input editor UI. */
    void DrawElement() override;

    /** @brief Refreshes cached mapping-ID tables and manages the rumble test timer. */
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
