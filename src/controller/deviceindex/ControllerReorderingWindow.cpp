#include "ControllerReorderingWindow.h"

namespace LUS {

ControllerReorderingWindow::~ControllerReorderingWindow() {
}

void ControllerReorderingWindow::InitElement() {
    mSDLIndex = -1;
}

void ControllerReorderingWindow::UpdateElement() {
}

void ControllerReorderingWindow::DrawElement() {
    // loop through and show a "press a button" modal for each controller connected (or total # of ports, whichever is lower)
    // use MAXCONTROLLERS not hardcoded 4
    // get a vector of int32_t (sdl index) from the modals
    // call into InitializeMappingsMultiplayer on the LUSDeviceIndexMappingManager with that vector
    // maybe have the thing the window calls remove the window? 
    // maybe just use the cvar window pattern and never remove the window, just do the standard don't show stuff


    ImGui::OpenPopup("Delete?");
    if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("All those beautiful files will be deleted.\nThis operation cannot be undone!");
        ImGui::Separator();

        //static int unused_i = 0;
        //ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");

        static bool dont_ask_me_next_time = false;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
        ImGui::PopStyleVar();

        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}

int32_t ControllerReorderingWindow::GetSDLIndex() {
    return mSDLIndex;
}

}