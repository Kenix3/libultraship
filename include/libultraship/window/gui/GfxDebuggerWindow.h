#pragma once

#include "ship/window/gui/GuiWindow.h"
#include <vector>
#include <memory>

namespace Fast {
union F3DGfx;
class Interpreter;
class GfxDebugger;
class Fast3dGui;
class Fast3dWindow;
} // namespace Fast

namespace Ship {
class ResourceManager;
} // namespace Ship

namespace LUS {

/**
 * @brief An ImGui window that visualises and inspects an N64 display list in real-time.
 *
 * GfxDebuggerWindow provides a tree-based disassembly view of the display list
 * currently being executed by the Fast3D interpreter. When the user requests
 * debugging (via the menu or GfxDebuggerRequestDebugging()), the window captures
 * a single frame's display list and renders an expandable node tree where each
 * node corresponds to one or more graphics microcode commands.
 *
 * The window is registered in the GUI as "Gfx Debugger" and is accessible via
 * Gui::GetGuiWindow("Gfx Debugger").
 */
class GfxDebuggerWindow : public Ship::GuiWindow {
  public:
    /**
     * @brief Constructs a GfxDebuggerWindow with constructor-injected dependencies.
     * @param consoleVariable  CVar name controlling window visibility.
     * @param name             Window title.
     * @param fast3dWindow     Fast3dWindow whose interpreter and GUI are used.
     * @param gfxDebugger      GfxDebugger used to capture/inspect display lists.
     * @param resourceManager  ResourceManager for archive lookups.
     */
    GfxDebuggerWindow(const std::string& consoleVariable, const std::string& name,
                      std::shared_ptr<Fast::Fast3dWindow> fast3dWindow, std::shared_ptr<Fast::GfxDebugger> gfxDebugger,
                      std::shared_ptr<Ship::ResourceManager> resourceManager);
    virtual ~GfxDebuggerWindow();

  protected:
    /** @brief Caches a weak reference to the Fast3D interpreter on first init. */
    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;

    /** @brief Polls the interpreter for a new display list when debugging is requested. */
    void UpdateElement() override;

    /** @brief Renders the disassembly tree for the captured display list. */
    void DrawElement() override;

  private:
    /**
     * @brief Recursively renders a single display-list command node.
     * @param cmd       Pointer to the F3DGfx command to display.
     * @param gfxPath   Full path from the root to @p cmd (for context/breadcrumbs).
     * @param parentPosY Y position of the parent node (used for connector lines).
     */
    void DrawDisasNode(const Fast::F3DGfx* cmd, std::vector<const Fast::F3DGfx*>& gfxPath, float parentPosY) const;

    /** @brief Renders the top-level disassembly tree for the last captured breakpoint. */
    void DrawDisas();

  private:
    std::vector<const Fast::F3DGfx*> mLastBreakPoint = {}; ///< Last captured display list command buffer.
    std::weak_ptr<Fast::Interpreter> mInterpreter; ///< Weak reference to the Fast3D interpreter (constructor-injected).
    std::shared_ptr<Fast::GfxDebugger> mGfxDebugger;         ///< GfxDebugger component (constructor-injected).
    std::shared_ptr<Fast::Fast3dGui> mFast3dGui;             ///< Fast3dGui reference (constructor-injected).
    std::shared_ptr<Ship::ResourceManager> mResourceManager; ///< ResourceManager component (constructor-injected).
};

} // namespace LUS
