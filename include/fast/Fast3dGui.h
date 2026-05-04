#pragma once
#include "ship/window/gui/Gui.h"

namespace Fast {

/**
 * @brief Concrete Gui subclass for the Fast3D rendering backend.
 *
 * Overrides the virtual ImGui backend methods defined by Ship::Gui with
 * implementations that dispatch to the appropriate platform/renderer backend
 * (SDL+OpenGL, SDL+Metal, or DXGI+DX11) based on the active WindowBackend.
 */
class Fast3dGui : public Ship::Gui {
  public:
    Fast3dGui();
    Fast3dGui(std::vector<std::shared_ptr<Ship::GuiWindow>> guiWindows);
    ~Fast3dGui() override = default;

    bool SupportsViewports() override;
    void HandleWindowEvents(Ship::WindowEvent event) override;

  protected:
    void ImGuiWMInit() override;
    void ImGuiWMShutdown() override;
    void ImGuiBackendInit() override;
    void ImGuiBackendShutdown() override;
    void ImGuiBackendNewFrame() override;
    void ImGuiWMNewFrame() override;
    void ImGuiRenderDrawData(ImDrawData* data) override;
    void DrawFloatingWindows() override;
};
} // namespace Fast
