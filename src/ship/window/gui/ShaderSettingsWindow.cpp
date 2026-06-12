#include "ship/window/gui/ShaderSettingsWindow.h"

#include "ship/Context.h"
#include "fast/Fast3dWindow.h"
#include "fast/interpreter.h"
#include <imgui.h>
#include <cstring>

namespace Ship {

void ShaderSettingsWindow::DrawElement() {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Context::GetInstance()->GetWindow());
    if (wnd == nullptr) {
        return;
    }
    auto interp = wnd->GetInterpreterWeak().lock();
    if (interp == nullptr) {
        return;
    }

    auto passes = interp->GetPostPasses();
    if (!passes.empty()) {
        ImGui::SeparatorText("Post-processing passes");
        for (const auto& pass : passes) {
            bool enabled = pass.enabled;
            const std::string label = (pass.pack.empty() ? pass.path : pass.pack) + "##pass" + std::to_string(pass.id);
            if (ImGui::Checkbox(label.c_str(), &enabled)) {
                interp->SetPostPassEnabled(pass.id, enabled);
            }
            if (!pass.pack.empty() && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", pass.path.c_str());
            }
        }
    }

    const auto& registry = interp->GetShaderSettingsRegistry();
    if (registry.empty()) {
        ImGui::TextDisabled("No tweakable shaders compiled yet.");
        ImGui::TextDisabled("Settings appear once a shader using @setting() is used.");
        return;
    }

    for (const auto& [shaderId, entry] : registry) {
        if (entry.decls.empty()) {
            continue;
        }
        const char* header =
            !entry.pack.empty() ? entry.pack.c_str() : (entry.path.empty() ? "(unnamed shader)" : entry.path.c_str());
        ImGui::SeparatorText(header);
        if (!entry.pack.empty()) {
            ImGui::TextDisabled("%s", entry.path.c_str());
        }
        ImGui::PushID((int)shaderId);
        for (const auto& decl : entry.decls) {
            float v[4];
            memcpy(v, entry.values.at(decl.var).data(), sizeof(v));
            const char* label = decl.name.empty() ? decl.var.c_str() : decl.name.c_str();

            if (decl.type == "toggle") {
                bool on = v[0] != 0.0f;
                if (ImGui::Checkbox(label, &on)) {
                    v[0] = on ? 1.0f : 0.0f;
                    interp->SetShaderSettingValue(shaderId, decl.var, v);
                }
            } else if (decl.type == "enum") {
                int current = 0;
                for (size_t i = 0; i < decl.optionValues.size(); i++) {
                    if (decl.optionValues[i] == v[0]) {
                        current = (int)i;
                        break;
                    }
                }
                if (ImGui::BeginCombo(label, decl.optionLabels[current].c_str())) {
                    for (size_t i = 0; i < decl.optionLabels.size(); i++) {
                        const bool selected = (int)i == current;
                        if (ImGui::Selectable(decl.optionLabels[i].c_str(), selected) && !selected) {
                            v[0] = decl.optionValues[i];
                            interp->SetShaderSettingValue(shaderId, decl.var, v);
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            } else if (decl.type == "int") {
                int iv = (int)v[0];
                if (ImGui::SliderInt(label, &iv, (int)decl.min, (int)decl.max)) {
                    v[0] = (float)iv;
                    interp->UpdateShaderSettingValue(shaderId, decl.var, v);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    interp->SetShaderSettingValue(shaderId, decl.var, v);
                }
            } else if (decl.type == "color") {
                if (ImGui::ColorEdit3(label, v)) {
                    interp->UpdateShaderSettingValue(shaderId, decl.var, v);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    interp->SetShaderSettingValue(shaderId, decl.var, v);
                }
            } else {
                if (ImGui::SliderFloat(label, &v[0], decl.min, decl.max)) {
                    interp->UpdateShaderSettingValue(shaderId, decl.var, v);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    interp->SetShaderSettingValue(shaderId, decl.var, v);
                }
            }
        }
        ImGui::PopID();
    }
}

} // namespace Ship
