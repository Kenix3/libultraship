#include "ControllerDisconnectedWindow.h"
#include <Utils/StringHelper.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include "Context.h"
#ifdef __WIIU__
#include <vpad/input.h>
#include <padscore/kpad.h>
#include "port/wiiu/WiiUImpl.h"
#endif

namespace LUS {

ControllerDisconnectedWindow::~ControllerDisconnectedWindow() {
}

void ControllerDisconnectedWindow::InitElement() {
    mPortIndexOfDisconnectedController = UINT8_MAX;
}

void ControllerDisconnectedWindow::UpdateElement() {
#ifndef __WIIU__
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
#endif
}

#ifdef __WIIU__
bool ControllerDisconnectedWindow::AnyWiiUDevicesAreConnected() {
    VPADReadError verror;
    VPADStatus* vstatus = LUS::WiiU::GetVPADStatus(&verror);

    if (vstatus != nullptr && verror == VPAD_READ_SUCCESS) {
        return true;
    }

    for (uint32_t channel = 0; channel < 4; channel++) {
        KPADError kerror;
        KPADStatus* kstatus = LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(channel), &kerror);

        if (kstatus != nullptr && kerror == KPAD_ERROR_OK) {
            return true;
        }
    }

    return false;
}

int32_t ControllerDisconnectedWindow::GetWiiUDeviceFromWiiUInput() {
    VPADReadError verror;
    VPADStatus* vstatus = LUS::WiiU::GetVPADStatus(&verror);

    if (vstatus != nullptr && verror == VPAD_READ_SUCCESS) {
        if (vstatus->hold) {
            // todo: don't just use INT32_MAX to mean gamepad
            return INT32_MAX;
        }
    }

    for (int32_t channel = 0; channel < 4; channel++) {
        KPADError kerror;
        KPADStatus* kstatus = LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(channel), &kerror);

        if (kstatus != nullptr && kerror == KPAD_ERROR_OK) {
            if (kstatus->hold) {
                return channel;
            }
        }
    }

    return -1;
}
#else
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
#endif

#ifndef __WIIU__
void ControllerDisconnectedWindow::DrawKnownControllerDisconnected() {
    ImGui::Text("Controller for port %d disconnected.\nPress any button or move any axis\non an unused controller "
                "for port %d.",
                mPortIndexOfDisconnectedController + 1, mPortIndexOfDisconnectedController + 1);

    auto index = GetSDLIndexFromSDLInput();
    if (index != -1 &&
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetLUSDeviceIndexFromSDLDeviceIndex(
            index) == LUSDeviceIndex::Max) {
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
#endif

#ifdef __WIIU__
void ControllerDisconnectedWindow::DrawUnknownOrMultipleControllersDisconnected() {
    ImGui::Text("Connected controller(s) have been added/removed/modified.");

    if (AnyWiiUDevicesAreConnected() && ImGui::Button("Reorder controllers###reorderControllersButton")) {
        auto reorderingWindow = Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Controller Reordering");
        if (reorderingWindow != nullptr) {
            mPortIndexOfDisconnectedController = UINT8_MAX;
            ImGui::CloseCurrentPopup();
            reorderingWindow->Show();
            Hide();
        }
    }
}
#else
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
#endif

#ifdef __WIIU__
void ControllerDisconnectedWindow::DrawElement() {
    // todo: don't use UINT8_MAX to mean we don't have a disconnected controller
    if (mPortIndexOfDisconnectedController == UINT8_MAX) {
        return;
    }

    ImGui::OpenPopup("Connected Controllers Changed");
    if (ImGui::BeginPopupModal("Connected Controllers Changed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        DrawUnknownOrMultipleControllersDisconnected();
        ImGui::EndPopup();
    }
}
#else
void ControllerDisconnectedWindow::DrawElement() {
    // todo: don't use UINT8_MAX to mean we don't have a disconnected controller
    if (mPortIndexOfDisconnectedController == UINT8_MAX) {
        return;
    }

    ImGui::OpenPopup("Controller Disconnected");
    if (ImGui::BeginPopupModal("Controller Disconnected", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        // todo: don't use UINT8_MAX-1 to mean we don't know what controller was disconnected
        if (mPortIndexOfDisconnectedController == UINT8_MAX - 1) {
            DrawUnknownOrMultipleControllersDisconnected();
        } else {
            DrawKnownControllerDisconnected();
        }
        ImGui::EndPopup();
    }
}
#endif

void ControllerDisconnectedWindow::SetPortIndexOfDisconnectedController(uint8_t portIndex) {
    // todo: don't use UINT8_MAX to mean we don't have a disconnected controller
    if (mPortIndexOfDisconnectedController == UINT8_MAX) {
        mPortIndexOfDisconnectedController = portIndex;
        return;
    }

    // todo: don't use UINT8_MAX-1 to mean multiple controllers disconnected
    mPortIndexOfDisconnectedController = UINT8_MAX - 1;
}
} // namespace LUS