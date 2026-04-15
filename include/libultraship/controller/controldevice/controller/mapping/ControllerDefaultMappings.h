#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"

namespace LUS {

/**
 * @brief N64-specific controller default mapping configuration.
 *
 * LUS::ControllerDefaultMappings is the concrete ControllerDefaultMappings
 * subclass used by libultraship. It provides the standard N64 button layout for
 * keyboard, SDL gamepad buttons, and SDL axis-to-button mappings so that newly
 * connected devices receive sensible default bindings out of the box.
 *
 * Pass an instance to LUS::ControlDeck (or Ship::ControlDeck) to customise the
 * default mappings applied when a new physical device is detected.
 */
class ControllerDefaultMappings : public Ship::ControllerDefaultMappings {
  public:
    /**
     * @brief Full constructor — supply all default mapping tables explicitly.
     *
     * @param defaultKeyboardKeyToButtonMappings          Button → keyboard key default bindings.
     * @param defaultKeyboardKeyToAxisDirectionMappings   Stick direction → keyboard key default bindings.
     * @param defaultSDLButtonToButtonMappings            Button → SDL gamepad button default bindings.
     * @param defaultSDLButtonToAxisDirectionMappings     Stick direction → SDL button default bindings.
     * @param defaultSDLAxisDirectionToButtonMappings     Button → SDL axis default bindings.
     * @param defaultSDLAxisDirectionToAxisDirectionMappings Stick direction → SDL axis default bindings.
     */
    ControllerDefaultMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<Ship::KbScancode>>
            defaultKeyboardKeyToButtonMappings,
        std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, Ship::KbScancode>>>
            defaultKeyboardKeyToAxisDirectionMappings,
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings,
        std::unordered_map<Ship::StickIndex, std::vector<std::pair<Ship::Direction, SDL_GameControllerButton>>>
            defaultSDLButtonToAxisDirectionMappings,
        std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
            defaultSDLAxisDirectionToButtonMappings,
        std::unordered_map<Ship::StickIndex,
                           std::vector<std::pair<Ship::Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
            defaultSDLAxisDirectionToAxisDirectionMappings);

    /**
     * @brief Constructs with an empty mapping table (all defaults are unset).
     *
     * Use this when you want to build defaults incrementally via the base-class setters.
     */
    ControllerDefaultMappings();
    ~ControllerDefaultMappings();

  private:
    /**
     * @brief Applies the given keyboard-to-button mapping table as the default.
     * @param defaultKeyboardKeyToButtonMappings Map from button bitmask to keyboard scan codes.
     */
    void
    SetDefaultKeyboardKeyToButtonMappings(std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<Ship::KbScancode>>
                                              defaultKeyboardKeyToButtonMappings) override;

    /**
     * @brief Applies the given SDL-button-to-button mapping table as the default.
     * @param defaultSDLButtonToButtonMappings Map from button bitmask to SDL gamepad buttons.
     */
    void SetDefaultSDLButtonToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings) override;

    /**
     * @brief Applies the given SDL-axis-to-button mapping table as the default.
     * @param defaultSDLAxisDirectionToButtonMappings Map from button bitmask to (SDL axis, direction) pairs.
     */
    void SetDefaultSDLAxisDirectionToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
            defaultSDLAxisDirectionToButtonMappings) override;
};
} // namespace LUS
