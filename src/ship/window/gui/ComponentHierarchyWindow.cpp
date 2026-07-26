#include "ship/window/gui/ComponentHierarchyWindow.h"

#include "ship/Context.h"
#include "ship/TickableComponent.h"

namespace Ship {

ComponentHierarchyWindow::ComponentHierarchyWindow(std::shared_ptr<ConsoleVariable> consoleVariable,
                                                   std::shared_ptr<Window> window, const std::string& visibilityCvar,
                                                   const std::string& name)
    : GuiWindow(std::move(consoleVariable), std::move(window), visibilityCvar, false, name, ImVec2{ 600, 400 },
                ImGuiWindowFlags_None) {
}

ComponentHierarchyWindow::~ComponentHierarchyWindow() {
}

void ComponentHierarchyWindow::DrawElement() {
    auto context = GetContext();
    if (!context) {
        ImGui::TextUnformatted("No context available.");
        return;
    }

    const uint64_t currentVersion = context->GetChildren().GetMutationVersion();
    if (currentVersion != mLastHierarchyVersion) {
        mCachedTreeString = context->ToTreeString();
        mLastHierarchyVersion = currentVersion;
    }

    if (ImGui::BeginChild("##component_tree", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true)) {
        ImGui::SeparatorText("Component Hierarchy");
        ImGui::TextUnformatted(mCachedTreeString.c_str());

        ImGui::Spacing();
        ImGui::SeparatorText("Tickable Components");
        for (const auto& tickable : *context->GetTickableComponents().Get()) {
            ImGui::BulletText("%s", tickable->GetName().c_str());
        }
    }
    ImGui::EndChild();
}

} // namespace Ship
