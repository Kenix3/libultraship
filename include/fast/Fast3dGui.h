#pragma once
#include "ship/window/gui/Gui.h"
#include "fast/WindowEvent.h"

// Fixes issue #926: HandleWindowEvents is only ever called from Fast3D backend code
// (gfx_sdl2.cpp, gfx_dxgi.cpp) and must not be a virtual method on Ship::Gui.
// The WindowEvent type has been moved to the Fast namespace so that ship code does
// not depend on any Fast3D or platform-specific types.

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

    /**
     * @brief Forwards a platform window event to the active ImGui backend.
     *
     * Only Fast3D backends (gfx_sdl2, gfx_dxgi) construct and dispatch WindowEvents.
     * Callers retrieve the Gui via Window::GetGui() and dynamic_cast to Fast3dGui.
     * @param event Platform event (SDL or Win32) wrapped in a Fast::WindowEvent.
     */
    void HandleWindowEvents(Fast::WindowEvent event);

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
