#include "StatsWindow.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include "public/bridge/consolevariablebridge.h"
#include "spdlog/spdlog.h"

namespace Ship {
StatsWindow::~StatsWindow() {
    SPDLOG_TRACE("destruct stats window");
}

void StatsWindow::InitElement() {
}

void StatsWindow::DrawElement() {
    const float framerate = ImGui::GetIO().Framerate;
    const float deltatime = ImGui::GetIO().DeltaTime;
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

#if defined(_WIN32)
    ImGui::Text("Platform: Windows");
#elif defined(__IOS__)
    ImGui::Text("Platform: iOS");
#elif defined(__APPLE__)
    ImGui::Text("Platform: macOS");
#elif defined(__linux__)
    ImGui::Text("Platform: Linux");
#else
    ImGui::Text("Platform: Unknown");
#endif
    ImGui::Text("Status: %.3f ms/frame (%.1f FPS)", deltatime * 1000.0f, framerate);
    ImGui::PopStyleColor();
}

void StatsWindow::UpdateElement() {
}
} // namespace Ship
