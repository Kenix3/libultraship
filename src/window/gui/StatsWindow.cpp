#include "StatsWindow.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "ImGui/imgui.h"
#include "public/bridge/consolevariablebridge.h"
#include "spdlog/spdlog.h"

namespace LUS {
StatsWindow::~StatsWindow() {
    SPDLOG_TRACE("destruct stats window");
}

void StatsWindow::InitElement() {
}

void StatsWindow::DrawElement() {
    const float framerate = ImGui::GetIO().Framerate;
    const float deltatime = ImGui::GetIO().DeltaTime;

    static int location = 0;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (location >= 0) {
        const float PAD = 10.0f;
        const float MENU_BAR_HEIGHT = 25.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        work_pos.y += MENU_BAR_HEIGHT;
        ImVec2 work_size = viewport->WorkSize;
        work_size.y -= MENU_BAR_HEIGHT;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoMove;
    } else if (location == -2) {
        // Center window
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        window_flags |= ImGuiWindowFlags_NoMove;
    }

    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

    if (ImGui::Begin("Example: Simple overlay", &mIsVisible, window_flags)) {

        ImGui::Text("Debug Stats\n"
                    "(right-click to change position)");

        #if defined(_WIN32)
        ImGui::Text("Platform: Windows");
#elif defined(__APPLE__)
        ImGui::Text("Platform: macOS");
#elif defined(__SWITCH__)
        ImGui::Text("Platform: Nintendo Switch");
#elif defined(__WIIU__)
        ImGui::Text("Platform: Nintendo Wii U");
#elif defined(__linux__)
        ImGui::Text("Platform: Linux");
#else
        ImGui::Text("Platform: Unknown");
#endif
        ImGui::Text("Status: %.3f ms/frame (%.1f FPS)", deltatime * 1000.0f, framerate);

        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom", NULL, location == -1))
                location = -1;
            if (ImGui::MenuItem("Center", NULL, location == -2))
                location = -2;
            if (ImGui::MenuItem("Top-left", NULL, location == 0))
                location = 0;
            if (ImGui::MenuItem("Top-right", NULL, location == 1))
                location = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, location == 2))
                location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3))
                location = 3;
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void StatsWindow::UpdateElement() {
}
} // namespace LUS
