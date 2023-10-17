#include "ControllerDisconnectedWindow.h"
#include <Utils/StringHelper.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include "Context.h"

namespace LUS {

ControllerDisconnectedWindow::~ControllerDisconnectedWindow() {
}

void ControllerDisconnectedWindow::InitElement() {
    mPortIndexOfDisconnectedController = UINT8_MAX;
}

void ControllerDisconnectedWindow::UpdateElement() {
}

int32_t ControllerDisconnectedWindow::GetSDLIndexFromSDLInput() {
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

void ControllerDisconnectedWindow::DrawElement() {
    // don't show the modal until we have an accurate port number to display
    if (mPortIndexOfDisconnectedController == UINT8_MAX) {
        return;
    }

    ImGui::OpenPopup("Controller Disconnected");
    if (ImGui::BeginPopupModal("Controller Disconnected", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Controller for port %d disconnected.\nPress any button or move any axis\non an unused controller "
                    "for port %d.",
                    mPortIndexOfDisconnectedController + 1, mPortIndexOfDisconnectedController + 1);

        auto index = GetSDLIndexFromSDLInput();
        if (index != -1 && Context::GetInstance()
                                   ->GetControlDeck()
                                   ->GetDeviceIndexMappingManager()
                                   ->GetLUSDeviceIndexFromSDLDeviceIndex(index) == LUSDeviceIndex::Max) {
            Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeSDLMappingsForPort(
                mPortIndexOfDisconnectedController, index);
            mPortIndexOfDisconnectedController = UINT8_MAX;
            ImGui::CloseCurrentPopup();
            Hide();
        }

        if (ImGui::Button(StringHelper::Sprintf("Play without controller connected to port %d", mPortIndexOfDisconnectedController + 1).c_str())) {
            mPortIndexOfDisconnectedController = UINT8_MAX;
            ImGui::CloseCurrentPopup();
            Hide();
        }

        ImGui::EndPopup();
    }
}

void ControllerDisconnectedWindow::SetPortIndexOfDisconnectedController(uint8_t portIndex) {
    mPortIndexOfDisconnectedController = portIndex;
}
} // namespace LUS