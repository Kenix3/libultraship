#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <SDL2/SDL.h>
#include "ControllerAxisDirectionMapping.h"

#ifndef CONTROLLERBUTTONS_T
#define CONTROLLERBUTTONS_T uint16_t
#endif

#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Holds the default input-to-button and input-to-axis mappings for a controller.
 *
 * Provides separate mapping tables for keyboard-key, SDL-gamepad-button, and
 * SDL-gamepad-axis input sources. Subclasses may override the protected setters to
 * customise the defaults for a specific game or controller layout.
 */
class ControllerDefaultMappings {
  public:
    /**
     * @brief Constructs a ControllerDefaultMappings with fully specified default tables.
     * @param defaultKeyboardKeyToButtonMappings              Keyboard key to button bitmask mappings.
     * @param defaultKeyboardKeyToAxisDirectionMappings        Keyboard key to axis direction mappings.
     * @param defaultSDLButtonToButtonMappings                 SDL gamepad button to button bitmask mappings.
     * @param defaultSDLButtonToAxisDirectionMappings           SDL gamepad button to axis direction mappings.
     * @param defaultSDLAxisDirectionToButtonMappings           SDL gamepad axis direction to button bitmask mappings.
     * @param defaultSDLAxisDirectionToAxisDirectionMappings    SDL gamepad axis direction to axis direction mappings.
     */
    ControllerDefaultMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings,
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
            defaultKeyboardKeyToAxisDirectionMappings,
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings,
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
            defaultSDLButtonToAxisDirectionMappings,
        std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
            defaultSDLAxisDirectionToButtonMappings,
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
            defaultSDLAxisDirectionToAxisDirectionMappings);

    /** @brief Constructs a ControllerDefaultMappings with empty default tables. */
    ControllerDefaultMappings();
    ~ControllerDefaultMappings();

    /**
     * @brief Returns the default keyboard-key-to-button mappings.
     * @return Map of button bitmask to set of keyboard scancodes.
     */
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> GetDefaultKeyboardKeyToButtonMappings();

    /**
     * @brief Returns the default keyboard-key-to-axis-direction mappings.
     * @return Map of stick index to direction/scancode pairs.
     */
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
    GetDefaultKeyboardKeyToAxisDirectionMappings();

    /**
     * @brief Returns the default SDL-button-to-button mappings.
     * @return Map of button bitmask to set of SDL gamepad buttons.
     */
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
    GetDefaultSDLButtonToButtonMappings();

    /**
     * @brief Returns the default SDL-button-to-axis-direction mappings.
     * @return Map of stick index to direction/SDL-button pairs.
     */
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
    GetDefaultSDLButtonToAxisDirectionMappings();

    /**
     * @brief Returns the default SDL-axis-direction-to-button mappings.
     * @return Map of button bitmask to axis/threshold pairs.
     */
    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
    GetDefaultSDLAxisDirectionToButtonMappings();

    /**
     * @brief Returns the default SDL-axis-direction-to-axis-direction mappings.
     * @return Map of stick index to direction/(axis, threshold) pairs.
     */
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
    GetDefaultSDLAxisDirectionToAxisDirectionMappings();

  protected:
    /**
     * @brief Replaces the default keyboard-key-to-button mappings.
     * @param defaultKeyboardKeyToButtonMappings The new mappings.
     */
    virtual void SetDefaultKeyboardKeyToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings);

    /**
     * @brief Replaces the default keyboard-key-to-axis-direction mappings.
     * @param defaultKeyboardKeyToAxisDirectionMappings The new mappings.
     */
    virtual void SetDefaultKeyboardKeyToAxisDirectionMappings(
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
            defaultKeyboardKeyToAxisDirectionMappings);

    /**
     * @brief Replaces the default SDL-button-to-button mappings.
     * @param defaultSDLButtonToButtonMappings The new mappings.
     */
    virtual void SetDefaultSDLButtonToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings);

    /**
     * @brief Replaces the default SDL-button-to-axis-direction mappings.
     * @param defaultSDLButtonToAxisDirectionMappings The new mappings.
     */
    virtual void SetDefaultSDLButtonToAxisDirectionMappings(
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
            defaultSDLButtonToAxisDirectionMappings);

    /**
     * @brief Replaces the default SDL-axis-direction-to-button mappings.
     * @param defaultSDLAxisDirectionToButtonMappings The new mappings.
     */
    virtual void SetDefaultSDLAxisDirectionToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
            defaultSDLAxisDirectionToButtonMappings);

    /**
     * @brief Replaces the default SDL-axis-direction-to-axis-direction mappings.
     * @param defaultSDLAxisDirectionToAxisDirectionMappings The new mappings.
     */
    virtual void SetDefaultSDLAxisDirectionToAxisDirectionMappings(
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
            defaultSDLAxisDirectionToAxisDirectionMappings);

  private:
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> mDefaultKeyboardKeyToButtonMappings;
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
        mDefaultKeyboardKeyToAxisDirectionMappings;
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
        mDefaultSDLButtonToButtonMappings;
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
        mDefaultSDLButtonToAxisDirectionMappings;
    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
        mDefaultSDLAxisDirectionToButtonMappings;
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
        mDefaultSDLAxisDirectionToAxisDirectionMappings;
};
} // namespace Ship
