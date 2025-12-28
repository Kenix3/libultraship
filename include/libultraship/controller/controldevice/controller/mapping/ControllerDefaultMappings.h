#pragma once

#include "ship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"

namespace LUS {

class ControllerDefaultMappings : public Ship::ControllerDefaultMappings {
  public:
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
    ControllerDefaultMappings();
    ~ControllerDefaultMappings();

  private:
    void
    SetDefaultKeyboardKeyToButtonMappings(std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<Ship::KbScancode>>
                                              defaultKeyboardKeyToButtonMappings) override;

    void SetDefaultSDLButtonToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::unordered_set<SDL_GameControllerButton>>
            defaultSDLButtonToButtonMappings) override;

    void SetDefaultSDLAxisDirectionToButtonMappings(
        std::unordered_map<CONTROLLERBUTTONS_T, std::vector<std::pair<SDL_GameControllerAxis, int32_t>>>
            defaultSDLAxisDirectionToButtonMappings) override;
};
} // namespace LUS
