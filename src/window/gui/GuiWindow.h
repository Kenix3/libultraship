#pragma once

#include <string>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>
#include "window/gui/GuiElement.h"

namespace Ship {
class GuiWindow : public GuiElement {
  public:
    GuiWindow() = default;
    GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name, ImVec2 originalSize, uint32_t windowFlags);
    GuiWindow(const std::string& consoleVariable, const std::string& name, ImVec2 originalSize, uint32_t windowFlags);
    void Draw() override;
    std::string GetName();

  protected:
    void BeginGroupPanel(const char* name, const ImVec2& size);
    void EndGroupPanel(float minHeight);

  private:
    void SyncVisibilityConsoleVariable();
    std::string mVisibilityConsoleVariable;
    std::string mName;
    ImVector<ImRect> mGroupPanelLabelStack;
    ImVec2 mOriginalSize;
    uint32_t mWindowFlags;
};
} // namespace Ship
