#include "ControllerReorderingWindow.h"
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

ControllerReorderingWindow::~ControllerReorderingWindow() {
}

void ControllerReorderingWindow::InitElement() {
    mDeviceIndices.clear();
    mCurrentPortNumber = 1;
}

void ControllerReorderingWindow::UpdateElement() {
}

#ifdef __WIIU__
std::vector<int32_t> ControllerReorderingWindow::GetConnectedWiiUDevices() {
    std::vector<int32_t> connectedDevices;

    VPADReadError verror;
    VPADStatus* vstatus = LUS::WiiU::GetVPADStatus(&verror);

    if (vstatus != nullptr && verror == VPAD_READ_SUCCESS) {
        connectedDevices.push_back(INT32_MAX);
    }

    for (uint32_t channel = 0; channel < 4; channel++) {
        KPADError kerror;
        KPADStatus* kstatus = LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(channel), &kerror);

        if (kstatus != nullptr && kerror == KPAD_ERROR_OK) {
            connectedDevices.push_back(channel);
        }
    }

    return connectedDevices;
}

int32_t ControllerReorderingWindow::GetWiiUDeviceFromWiiUInput() {
    VPADReadError verror;
    VPADStatus* vstatus = LUS::WiiU::GetVPADStatus(&verror);

    if (vstatus != nullptr && verror == VPAD_READ_SUCCESS) {
        if (vstatus->hold) {
            // todo: don't just use INT32_MAX to mean gamepad
            return INT32_MAX;
        }
        // todo: use this for getting buttons,
        // for (uint32_t i = VPAD_BUTTON_SYNC; i <= VPAD_STICK_L_EMULATION_LEFT; i <<= 1) {
        //     if (vstatus->hold == i) {

        //     }
        // }
    }

    for (int32_t channel = 0; channel < 4; channel++) {
        KPADError kerror;
        KPADStatus* kstatus = LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(channel), &kerror);

        if (kstatus != nullptr && kerror == KPAD_ERROR_OK) {
            if (kstatus->hold) {
                return channel;
            }

            switch (kstatus->extensionType) {
                case WPAD_EXT_NUNCHUK:
                case WPAD_EXT_MPLUS_NUNCHUK:
                    if (kstatus->nunchuck.hold) {
                        return channel;
                    }
                    break;
                case WPAD_EXT_CLASSIC:
                case WPAD_EXT_MPLUS_CLASSIC:
                    if (kstatus->classic.hold) {
                        return channel;
                    }
                    break;
                case WPAD_EXT_PRO_CONTROLLER:
                    if (kstatus->pro.hold) {
                        return channel;
                    }
                    break;
            }
        }
    }

    return -1;
}

void ControllerReorderingWindow::DrawElement() {
    // if we don't have more than one controller, just close the window
    auto connectedDevices = GetConnectedWiiUDevices();

    if (connectedDevices.size() <= 1) {
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeMappingsMultiplayer(
            connectedDevices);
        Hide();
        return;
    }

    // we have more than one controller, prompt the user for order
    if (mCurrentPortNumber <= std::min(connectedDevices.size(), static_cast<size_t>(MAXCONTROLLERS))) {
        ImGui::OpenPopup("Set Controller");
        if (ImGui::BeginPopupModal("Set Controller", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Press any button or move any axis\non the controller for port %d", mCurrentPortNumber);

            auto index = GetWiiUDeviceFromWiiUInput();
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
#else
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
    if (Context::GetInstance()->GetControlDeck()->IsSinglePlayerMappingMode()) {
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeMappingsSinglePlayer();
        Hide();
        return;
    }

    // if we don't have more than one controller, just close the window
    std::vector<int32_t> connectedSdlControllerIndices;
    for (auto i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
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
#endif
} // namespace LUS