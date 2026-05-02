#pragma once
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "FastMouseStateManager.h"

namespace Ship {
class ConsoleVariable;
class ControlDeck;
} // namespace Ship

union Gfx;
#include "interpreter.h"

namespace Fast {

/**
 * @brief Fast3D-based window and rendering context.
 *
 * Fast3dWindow drives the Fast3D graphics pipeline and integrates with the
 * Ship component hierarchy. The following components must be present as
 * **direct children of the Context** before Fast3dWindow is used:
 *
 *  - **Fast3dWindow** itself (Ship::Window) — registered so that keyboard/mouse
 *    event handlers can look up the active window.
 *  - **Ship::Config** — queried by Fast3dWindow and the DXGI/DX11 back-ends for
 *    persistent window settings (resolution, fullscreen, etc.).
 *  - **Ship::ConsoleVariable** — read by all Fast3D back-ends for runtime
 *    rendering toggles.
 *  - **Ship::ResourceManager** — required by the DX11 and OpenGL back-ends for
 *    shader/texture resource loading.
 *  - **Ship::FileDropMgr** — required by the DXGI back-end to handle file-drop
 *    events forwarded from the OS.
 *  - **Ship::ControlDeck** — required for keyboard and mouse input routing.
 *  - **Fast::GfxDebugger** — required by the interpreter for debug-draw mode.
 */
class Fast3dWindow : public Ship::Window {
  public:
    Fast3dWindow();
    Fast3dWindow(std::vector<std::shared_ptr<Ship::GuiWindow>> guiWindows);
    Fast3dWindow(std::shared_ptr<Ship::Gui> gui);
    Fast3dWindow(std::shared_ptr<Ship::Gui> gui, std::shared_ptr<FastMouseStateManager> mouseStateManager);
    ~Fast3dWindow();

    void Close() override;
    void RunGuiOnly() override;
    void StartFrame() override;
    void EndFrame() override;
    bool IsFrameReady() override;
    void HandleEvents() override;
    void SetCursorVisibility(bool visible) override;
    uint32_t GetWidth() override;
    uint32_t GetHeight() override;
    int32_t GetPosX() override;
    int32_t GetPosY() override;
    float GetAspectRatio() override;
    void SetMousePos(Ship::Coords pos) override;
    Ship::Coords GetMousePos() override;
    Ship::Coords GetMouseDelta() override;
    Ship::CoordsF GetMouseWheel() override;
    bool GetMouseState(Ship::MouseBtn btn) override;
    void SetMouseCapture(bool capture) override;
    bool IsMouseCaptured() override;
    uint32_t GetCurrentRefreshRate() override;
    bool SupportsWindowedFullscreen() override;
    bool CanDisableVerticalSync() override;
    void SetResolutionMultiplier(float multiplier) override;
    void SetMsaaLevel(uint32_t value) override;
    void SetFullscreen(bool isFullscreen) override;
    bool IsFullscreen() override;
    bool IsRunning() override;
    uintptr_t GetGfxFrameBuffer() override;
    const char* GetKeyName(int32_t scancode) override;

    void InitWindowManager();
    int32_t GetTargetFps();
    void SetTargetFps(int32_t fps);
    void SetMaximumFrameLatency(int32_t latency);
    void GetPixelDepthPrepare(float x, float y);
    uint16_t GetPixelDepth(float x, float y);
    void SetTextureFilter(FilteringMode filteringMode);
    void SetRendererUCode(UcodeHandlers ucode);
    void EnableSRGBMode();
    bool DrawAndRunGraphicsCommands(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtxReplacements);

    std::weak_ptr<Interpreter> GetInterpreterWeak() const;

  protected:
    void OnInit() override;

    static bool KeyDown(int32_t scancode);
    static bool KeyUp(int32_t scancode);
    static void AllKeysUp();
    static bool MouseButtonDown(int button);
    static bool MouseButtonUp(int button);
    static void OnFullscreenChanged(bool isNowFullscreen);

  private:
    GfxRenderingAPI* mRenderingApi;
    GfxWindowBackend* mWindowManagerApi;
    std::shared_ptr<Interpreter> mInterpreter = nullptr;
    std::shared_ptr<Ship::ConsoleVariable> mConsoleVariables;
    std::shared_ptr<Ship::ControlDeck> mControlDeck;

    /** @brief Returns the cached ConsoleVariable component. */
    std::shared_ptr<Ship::ConsoleVariable> GetConsoleVariables() const;
    /** @brief Returns the cached ControlDeck component. */
    std::shared_ptr<Ship::ControlDeck> GetControlDeck() const;
};
} // namespace Fast
