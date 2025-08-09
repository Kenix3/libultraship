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

#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

class ControllerDefaultMappings {
  public:
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
            defaultSDLAxisDirectionToAxisDirectionMappings,
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<CONTROLLERBUTTONS_T>>
            defaultGCAdapterButtonToButtonMappings,
        std::unordered_map<StickIndex, std::unordered_map<Direction, std::pair<uint8_t, int32_t>>>
            defaultGCAdapterAxisDirectionToAxisDirectionMappings);
    ControllerDefaultMappings();
    ~ControllerDefaultMappings();

    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> GetDefaultKeyboardKeyToButtonMappings();
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
    GetDefaultKeyboardKeyToAxisDirectionMappings();

    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
    GetDefaultSDLButtonToButtonMappings();
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
    GetDefaultSDLButtonToAxisDirectionMappings();

    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
    GetDefaultSDLAxisDirectionToButtonMappings();
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
    GetDefaultSDLAxisDirectionToAxisDirectionMappings();

    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<CONTROLLERBUTTONS_T>>
    GetDefaultGCAdapterButtonToButtonMappings();
    std::unordered_map<StickIndex, std::unordered_map<Direction, std::pair<uint8_t, int32_t>>>&
    GetDefaultGCAdapterAxisDirectionToAxisDirectionMappings();

  private:
    void SetDefaultKeyboardKeyToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings);
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> mDefaultKeyboardKeyToButtonMappings;

    void SetDefaultKeyboardKeyToAxisDirectionMappings(
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
            defaultKeyboardKeyToAxisDirectionMappings);
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, KbScancode>>>
        mDefaultKeyboardKeyToAxisDirectionMappings;

    void SetDefaultSDLButtonToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings);
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
        mDefaultSDLButtonToButtonMappings;

    void SetDefaultSDLButtonToAxisDirectionMappings(
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
            defaultSDLButtonToAxisDirectionMappings);
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, SDL_GameControllerButton>>>
        mDefaultSDLButtonToAxisDirectionMappings;

    void SetDefaultSDLAxisDirectionToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
            defaultSDLAxisDirectionToButtonMappings);
    std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
        mDefaultSDLAxisDirectionToButtonMappings;

    void SetDefaultSDLAxisDirectionToAxisDirectionMappings(
        std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
            defaultSDLAxisDirectionToAxisDirectionMappings);
    std::unordered_map<StickIndex, std::vector<std::pair<Direction, std::pair<SDL_GameControllerAxis, int32_t>>>>
        mDefaultSDLAxisDirectionToAxisDirectionMappings;

    void SetDefaultGCAdapterButtonToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<CONTROLLERBUTTONS_T>>
            defaultGCAdapterButtonToButtonMappings);
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<CONTROLLERBUTTONS_T>>
        mDefaultGCAdapterButtonToButtonMappings;

    void SetDefaultGCAdapterAxisDirectionToAxisDirectionMappings(
        std::unordered_map<StickIndex, std::unordered_map<Direction, std::pair<uint8_t, int32_t>>>
            defaultGCAdapterAxisDirectionToAxisDirectionMappings);
    std::unordered_map<StickIndex, std::unordered_map<Direction, std::pair<uint8_t, int32_t>>>
        mDefaultGCAdapterAxisDirectionToAxisDirectionMappings;
};
} // namespace Ship
