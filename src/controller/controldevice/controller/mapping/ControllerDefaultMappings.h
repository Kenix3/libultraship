#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <SDL2/SDL.h>

#ifndef CONTROLLERBUTTONS_T
#define CONTROLLERBUTTONS_T uint16_t
#endif

#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

class ControllerDefaultMappings {
  public:
    ControllerDefaultMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings,
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings);
    ControllerDefaultMappings();
    ~ControllerDefaultMappings();

    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> GetDefaultKeyboardKeyToButtonMappings();
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
    GetDefaultSDLButtonToButtonMappings();

  private:
    void SetDefaultKeyboardKeyToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> defaultKeyboardKeyToButtonMappings);
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<KbScancode>> mDefaultKeyboardKeyToButtonMappings;

    void SetDefaultSDLButtonToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings);
    std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
        mDefaultSDLButtonToButtonMappings;
};
} // namespace Ship
