#include "ControllerReorderingWindow.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include "Context.h"

namespace LUS {

ControllerReorderingWindow::~ControllerReorderingWindow() {
}

void ControllerReorderingWindow::InitElement() {
    mSDLDeviceIndices.clear();
    mCurrentPortNumber = 1;
}

void ControllerReorderingWindow::UpdateElement() {
}

int32_t ControllerReorderingWindow::GetSDLIndexFromSDLInput() {
    int32_t sdlDeviceIndex = -1;
    
    std::unordered_map<int32_t, SDL_GameController*> sdlControllers;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sdlControllers[i] = SDL_GameControllerOpen(i);
        }
    }

    for (auto [controllerIndex, controller] : sdlControllers) {
        for (int32_t button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++) {
            if (SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button))) {
                sdlDeviceIndex = controllerIndex;
                break;
            }
        }

        if (sdlDeviceIndex != -1) {
            break;
        }

        for (int32_t i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            const auto axis = static_cast<SDL_GameControllerAxis>(i);
            const auto axisValue = SDL_GameControllerGetAxis(controller, axis) / 32767.0f;
            if (axisValue < -0.7f || axisValue > 0.7f) {
                sdlDeviceIndex = controllerIndex;
                break;
            }
        }
    }

    for (auto [i, controller] : sdlControllers) {
        SDL_GameControllerClose(controller);
    }

    return sdlDeviceIndex;
}

void ControllerReorderingWindow::DrawElement() {
    // if we don't have more than one controller, just close the window
    int32_t sdlControllerCount = 0;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            sdlControllerCount++;
        }
    }
    if (sdlControllerCount <= 1) {
        Hide();
        return;
    }

    // we have more than one controller, prompt the user for order
    if (mCurrentPortNumber <= std::min(sdlControllerCount, MAXCONTROLLERS)) {
        ImGui::OpenPopup("Set Controller");
        if (ImGui::BeginPopupModal("Set Controller", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Press any button or move any axis\non the controller for port %d", mCurrentPortNumber);
            
            auto index = GetSDLIndexFromSDLInput();
            if (index != -1 && std::find(mSDLDeviceIndices.begin(), mSDLDeviceIndices.end(), index) == mSDLDeviceIndices.end()) {
                mSDLDeviceIndices.push_back(index);
                mCurrentPortNumber++;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        return;
    }

    Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeMappingsMultiplayer(mSDLDeviceIndices);
    mSDLDeviceIndices.clear();
    mCurrentPortNumber = 1;
    Hide();
}
}