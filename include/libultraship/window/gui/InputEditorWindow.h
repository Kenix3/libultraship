#pragma once

#include "stdint.h"
#include "ship/window/gui/GuiWindow.h"
#include <imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include "ship/controller/controldevice/controller/Controller.h"

namespace LUS {

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

    /** @brief Destroys the InputEditorWindow and releases any active rumble test state. */
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
    /**
     * @brief Draws a single stick-direction mapping line (e.g. "Stick Up").
     * @param axisDirectionName Human-readable axis/direction label.
     * @param port              Controller port index.
     * @param stick             Stick index (0 = left, 1 = right).
     * @param direction         Cardinal direction of the axis.
     * @param color             Colour used for the input chip.
     */
    void DrawStickDirectionLine(const char* axisDirectionName, uint8_t port, uint8_t stick, Ship::Direction direction,
                                ImVec4 color);

    /**
     * @brief Draws a single button mapping line showing current bindings.
     * @param buttonName Human-readable button label.
     * @param port       Controller port index.
     * @param bitmask    Button bitmask identifying the button.
     * @param color      Colour used for the input chip.
     */
    void DrawButtonLine(const char* buttonName, uint8_t port, CONTROLLERBUTTONS_T bitmask, ImVec4 color);

    /**
     * @brief Draws the edit/remove button for an existing button mapping.
     * @param port    Controller port index.
     * @param bitmask Button bitmask identifying the button.
     * @param id      Mapping identifier string.
     */
    void DrawButtonLineEditMappingButton(uint8_t port, CONTROLLERBUTTONS_T bitmask, std::string id);

    /**
     * @brief Draws the "+" button that opens the mapping-add popup for a button.
     * @param port    Controller port index.
     * @param bitmask Button bitmask identifying the button.
     */
    void DrawButtonLineAddMappingButton(uint8_t port, CONTROLLERBUTTONS_T bitmask);

    /**
     * @brief Draws the edit/remove button for an existing stick-direction mapping.
     * @param port      Controller port index.
     * @param stick     Stick index (0 = left, 1 = right).
     * @param direction Cardinal direction of the axis.
     * @param id        Mapping identifier string.
     */
    void DrawStickDirectionLineEditMappingButton(uint8_t port, uint8_t stick, Ship::Direction direction,
                                                 std::string id);

    /**
     * @brief Draws the "+" button that opens the mapping-add popup for a stick direction.
     * @param port      Controller port index.
     * @param stick     Stick index (0 = left, 1 = right).
     * @param direction Cardinal direction of the axis.
     */
    void DrawStickDirectionLineAddMappingButton(uint8_t port, uint8_t stick, Ship::Direction direction);

    /**
     * @brief Draws the complete analog-stick section (direction lines, preview, and sensitivity controls).
     * @param port  Controller port index.
     * @param stick Stick index (0 = left, 1 = right).
     * @param id    ImGui identifier used to distinguish collapsing headers.
     * @param color Colour used for input chips in this section.
     */
    void DrawStickSection(uint8_t port, uint8_t stick, int32_t id, ImVec4 color);

    /**
     * @brief Draws the rumble configuration section for a controller port.
     * @param port Controller port index.
     */
    void DrawRumbleSection(uint8_t port);

    /**
     * @brief Draws a remove button for an existing rumble mapping.
     * @param port Controller port index.
     * @param id   Mapping identifier string.
     */
    void DrawRemoveRumbleMappingButton(uint8_t port, std::string id);

    /**
     * @brief Draws the "+" button that opens the mapping-add popup for rumble.
     * @param port Controller port index.
     */
    void DrawAddRumbleMappingButton(uint8_t port);

    /**
     * @brief Draws the LED colour configuration section for a controller port.
     * @param port Controller port index.
     */
    void DrawLEDSection(uint8_t port);

    /**
     * @brief Draws a remove button for an existing LED mapping.
     * @param port Controller port index.
     * @param id   Mapping identifier string.
     */
    void DrawRemoveLEDMappingButton(uint8_t port, std::string id);

    /**
     * @brief Draws the "+" button that opens the mapping-add popup for LED.
     * @param port Controller port index.
     */
    void DrawAddLEDMappingButton(uint8_t port);

    /**
     * @brief Draws the gyroscope configuration section for a controller port.
     * @param port Controller port index.
     */
    void DrawGyroSection(uint8_t port);

    /**
     * @brief Draws a remove button for an existing gyro mapping.
     * @param port Controller port index.
     * @param id   Mapping identifier string.
     */
    void DrawRemoveGyroMappingButton(uint8_t port, std::string id);

    /**
     * @brief Draws the "+" button that opens the mapping-add popup for gyro.
     * @param port Controller port index.
     */
    void DrawAddGyroMappingButton(uint8_t port);

    int32_t mGameInputBlockTimer;
    int32_t mMappingInputBlockTimer;
    int32_t mRumbleTimer;
    std::shared_ptr<ControllerRumbleMapping> mRumbleMappingToTest;

    // mBitmaskToMappingIds[port][bitmask] = { id0, id1, ... }
    std::unordered_map<uint8_t, std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::string>>> mBitmaskToMappingIds;

    // mStickDirectionToMappingIds[port][stick][direction] = { id0, id1, ... }
    std::unordered_map<uint8_t,
                       std::unordered_map<uint8_t, std::unordered_map<Ship::Direction, std::vector<std::string>>>>
        mStickDirectionToMappingIds;

    /**
     * @brief Rebuilds the cached bitmask-to-mapping-ID table for a port.
     * @param port Controller port index.
     */
    void UpdateBitmaskToMappingIds(uint8_t port);

    /**
     * @brief Rebuilds the cached stick-direction-to-mapping-ID table for a port.
     * @param port Controller port index.
     */
    void UpdateStickDirectionToMappingIds(uint8_t port);

    /**
     * @brief Resolves button colours based on the physical device type of a mapping.
     * @param physicalDeviceType The device type to look up colours for.
     * @param buttonColor        Output normal-state colour.
     * @param buttonHoveredColor Output hovered-state colour.
     */
    void GetButtonColorsForPhysicalDeviceType(Ship::PhysicalDeviceType physicalDeviceType, ImVec4& buttonColor,
                                              ImVec4& buttonHoveredColor);

    /**
     * @brief Draws the full content of a single controller-port tab.
     * @param portIndex Controller port index.
     */
    void DrawPortTab(uint8_t portIndex);

    std::set<CONTROLLERBUTTONS_T> mButtonsBitmasks;
    std::set<CONTROLLERBUTTONS_T> mDpadBitmasks;
    bool mInputEditorPopupOpen;

    /**
     * @brief Draws the "Set Defaults" button that restores default mappings for a port.
     * @param portIndex Controller port index.
     */
    void DrawSetDefaultsButton(uint8_t portIndex);

    /**
     * @brief Draws the "Clear All" button that removes every mapping for a port.
     * @param portIndex Controller port index.
     */
    void DrawClearAllButton(uint8_t portIndex);

    /**
     * @brief Draws per-device-type enable/disable toggles for a port.
     * @param portIndex Controller port index.
     */
    void DrawDeviceToggles(uint8_t portIndex);

    /** @brief Adjusts the position of the active mapping popup so it stays on-screen. */
    void OffsetMappingPopup();
};
} // namespace LUS
