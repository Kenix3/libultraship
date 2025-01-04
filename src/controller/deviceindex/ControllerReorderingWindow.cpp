#include "ControllerReorderingWindow.h"
#include "utils/StringHelper.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include "Context.h"

namespace Ship {

ControllerReorderingWindow::~ControllerReorderingWindow() {
}

void ControllerReorderingWindow::InitElement() {
    mDeviceIndices.clear();
    mCurrentPortNumber = 1;
}

void ControllerReorderingWindow::UpdateElement() {
}

int32_t ControllerReorderingWindow::GetSDLIndexFromSDLInput() {
    int32_t sdlDeviceIndex = -1;

    std::unordered_map<int32_t, SDL_Gamepad*> sdlControllers;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGamepad(i)) {
            sdlControllers[i] = SDL_OpenGamepad(i);
        }
    }

    for (auto [controllerIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_GAMEPAD_BUTTON_SOUTH; button < SDL_GAMEPAD_BUTTON_COUNT; button++) {
            if (SDL_GetGamepadButton(controller, static_cast<SDL_GamepadButton>(button))) {
                sdlDeviceIndex = controllerIndex;
                break;
            }
        }

        if (sdlDeviceIndex != -1) {
            break;
        }

        for (int32_t i = SDL_GAMEPAD_AXIS_LEFTX; i < SDL_GAMEPAD_AXIS_COUNT; i++) {
            const auto axis = static_cast<SDL_GamepadAxis>(i);
            const auto axisValue = SDL_GetGamepadAxis(controller, axis) / 32767.0f;
            if (axisValue < -0.7f || axisValue > 0.7f) {
                sdlDeviceIndex = controllerIndex;
                break;
            }
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_CloseGamepad(controller);
    }

    return sdlDeviceIndex;
}

void ControllerReorderingWindow::DrawElement() {
    if (Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeMappingsSinglePlayer();
        Hide();
        return;
    }

    // if we don't have more than one controller, just close the window
    std::vector<int32_t> connectedSdlControllerIndices;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGamepad(i)) {
            connectedSdlControllerIndices.push_back(i);
        }
    }
    if (connectedSdlControllerIndices.size() <= 1) {
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeMappingsMultiplayer(
            connectedSdlControllerIndices);
        Hide();
        return;
    }

    // we have more than one controller, prompt the user for order
    if (mCurrentPortNumber <= std::min(connectedSdlControllerIndices.size(), static_cast<size_t>(MAXCONTROLLERS))) {
        ImGui::OpenPopup("Set Controller");
        if (ImGui::BeginPopupModal("Set Controller", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Press any button or move any axis\non the controller for port %d", mCurrentPortNumber);

            auto index = GetSDLIndexFromSDLInput();
            if (index != -1 && std::find(mDeviceIndices.begin(), mDeviceIndices.end(), index) == mDeviceIndices.end()) {
                mDeviceIndices.push_back(index);
                mCurrentPortNumber++;
                ImGui::CloseCurrentPopup();
            }

            if (mCurrentPortNumber > 1 &&
                ImGui::Button(StringHelper::Sprintf("Play with %d controller%s", mCurrentPortNumber - 1,
                                                    mCurrentPortNumber > 2 ? "s" : "")
                                  .c_str())) {
                mCurrentPortNumber =
                    MAXCONTROLLERS + 1; // stop showing modals, it will be reset to 1 after the mapping init stuff
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        return;
    }

    Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeMappingsMultiplayer(
        mDeviceIndices);
    mDeviceIndices.clear();
    mCurrentPortNumber = 1;
    Hide();
}
} // namespace Ship