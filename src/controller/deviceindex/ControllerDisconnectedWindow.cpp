#include "ControllerDisconnectedWindow.h"
#include "utils/StringHelper.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include "Context.h"

namespace Ship {

ControllerDisconnectedWindow::~ControllerDisconnectedWindow() {
}

void ControllerDisconnectedWindow::InitElement() {
    mPortIndexOfDisconnectedController = UINT8_MAX;
}

void ControllerDisconnectedWindow::UpdateElement() {
    SDL_PumpEvents();
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEADDED, SDL_CONTROLLERDEVICEADDED) > 0) {
        // from https://wiki.libsdl.org/SDL2/SDL_ControllerDeviceEvent: which - the joystick device index for
        // the SDL_CONTROLLERDEVICEADDED event
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->HandlePhysicalDeviceConnect(
            event.cdevice.which);
    }

    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEREMOVED, SDL_CONTROLLERDEVICEREMOVED) > 0) {
        // from https://wiki.libsdl.org/SDL2/SDL_ControllerDeviceEvent: which - the [...] instance id for the
        // SDL_CONTROLLERDEVICEREMOVED [...] event
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->HandlePhysicalDeviceDisconnect(
            event.cdevice.which);
    }
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

void ControllerDisconnectedWindow::DrawKnownControllerDisconnected() {
    ImGui::Text("Controller for port %d disconnected.\nPress any button or move any axis\non an unused controller "
                "for port %d.",
                mPortIndexOfDisconnectedController + 1, mPortIndexOfDisconnectedController + 1);

    auto index = GetSDLIndexFromSDLInput();
    if (index != -1 &&
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetShipDeviceIndexFromSDLDeviceIndex(
            index) == ShipDeviceIndex::Max) {
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeSDLMappingsForPort(
            mPortIndexOfDisconnectedController, index);
        mPortIndexOfDisconnectedController = UINT8_MAX;
        ImGui::CloseCurrentPopup();
        Hide();
    }

    if (ImGui::Button(StringHelper::Sprintf("Play without controller connected to port %d",
                                            mPortIndexOfDisconnectedController + 1)
                          .c_str())) {
        mPortIndexOfDisconnectedController = UINT8_MAX;
        ImGui::CloseCurrentPopup();
        Hide();
    }

    uint8_t connectedSdlControllerCount = 0;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            connectedSdlControllerCount++;
        }
    }

    if (connectedSdlControllerCount != 0 &&
        ImGui::Button(connectedSdlControllerCount > 1
                          ? "Reorder all controllers###reorderControllersButton"
                          : "Use connected controller for port 1###reorderControllersButton")) {
        mPortIndexOfDisconnectedController = UINT8_MAX;
        ImGui::CloseCurrentPopup();
        Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Reordering")->Show();
        Hide();
    }
}

void ControllerDisconnectedWindow::DrawUnknownOrMultipleControllersDisconnected() {
    ImGui::Text("Controller(s) disconnected.");

    uint8_t connectedSdlControllerCount = 0;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            connectedSdlControllerCount++;
        }
    }

    if (connectedSdlControllerCount != 0 &&
        ImGui::Button(connectedSdlControllerCount > 1
                          ? "Reorder all controllers###reorderControllersButton"
                          : "Use connected controller for port 1###reorderControllersButton")) {
        mPortIndexOfDisconnectedController = UINT8_MAX;
        ImGui::CloseCurrentPopup();
        Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Reordering")->Show();
        Hide();
    }
}

void ControllerDisconnectedWindow::DrawElement() {
    // todo: don't use UINT8_MAX to mean we don't have a disconnected controller
    if (mPortIndexOfDisconnectedController == UINT8_MAX) {
        return;
    }

    if (mPortIndexOfDisconnectedController == UINT8_MAX - 1) {
        DrawUnknownOrMultipleControllersDisconnected();
    } else {
        DrawKnownControllerDisconnected();
    }
}

void ControllerDisconnectedWindow::SetPortIndexOfDisconnectedController(uint8_t portIndex) {
    // todo: don't use UINT8_MAX to mean we don't have a disconnected controller
    if (mPortIndexOfDisconnectedController == UINT8_MAX) {
        mPortIndexOfDisconnectedController = portIndex;
        return;
    }

    // todo: don't use UINT8_MAX-1 to mean multiple controllers disconnected
    mPortIndexOfDisconnectedController = UINT8_MAX - 1;
}
} // namespace Ship