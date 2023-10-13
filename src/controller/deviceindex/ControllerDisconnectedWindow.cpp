#include "ControllerDisconnectedWindow.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include "Context.h"

namespace LUS {

ControllerDisconnectedWindow::~ControllerDisconnectedWindow() {
}

void ControllerDisconnectedWindow::InitElement() {
    mPortIndexOfDisconnectedController = UINT8_MAX;
    mSDLIndexOfNewController = -1;
}

void ControllerDisconnectedWindow::UpdateElement() {
}

void ControllerDisconnectedWindow::DrawElement() {
    // don't show the modal until we have an accurate port number to display
    if (mPortIndexOfDisconnectedController == UINT8_MAX) {
        return;
    }

    ImGui::OpenPopup("Controller Disconnected");
    if (ImGui::BeginPopupModal("Controller Disconnected", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Controller for port %d disconnected.\nPlease connect a controller to use for port %d.", mPortIndexOfDisconnectedController + 1, mPortIndexOfDisconnectedController + 1);
        
        if (mSDLIndexOfNewController != -1) {
            Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->InitializeSDLMappingsForPort(mPortIndexOfDisconnectedController, mSDLIndexOfNewController);
            mSDLIndexOfNewController = -1;
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

void ControllerDisconnectedWindow::SetSDLIndexOfNewController(int32_t sdlIndex) {
    mSDLIndexOfNewController = sdlIndex;
}
}