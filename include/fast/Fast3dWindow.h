#pragma once
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "FastMouseStateManager.h"
#include "fast/debug/GfxDebugger.h"

union Gfx;
#include "interpreter.h"

namespace Fast {

/**
 * @brief Identifies the graphics/windowing backend used by Fast3dWindow.
 *
 * These are the positive backend IDs registered by Fast3dWindow.
 * The general convention for window backend IDs (int32_t) is:
 *   negative  — no Window backend available (e.g. Window is not initialized)
 *   zero      — no Window backend in use
 *   positive  — backend defined by the Window subclass
 */
enum WindowBackend {
    FAST3D_DXGI_DX11 = 1,
    FAST3D_SDL_OPENGL = 2,
    FAST3D_SDL_METAL = 3,
};

class Fast3dWindow : public Ship::Window {
  public:
    Fast3dWindow();
    Fast3dWindow(std::vector<std::shared_ptr<Ship::GuiWindow>> guiWindows);
    Fast3dWindow(std::shared_ptr<Ship::Gui> gui);
    Fast3dWindow(std::shared_ptr<Ship::Gui> gui, std::shared_ptr<FastMouseStateManager> mouseStateManager);
    ~Fast3dWindow();

    void Init() override;
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

    std::string GetWindowBackendName() override;

    void SetCurrentDimensions(uint32_t width, uint32_t height) override;
    void SetCurrentDimensions(uint32_t width, uint32_t height, int32_t posX, int32_t posY) override;
    void SetCurrentDimensions(bool isFullscreen, uint32_t width, uint32_t height) override;
    void SetCurrentDimensions(bool isFullscreen, uint32_t width, uint32_t height, int32_t posX, int32_t posY) override;
    Ship::WindowRect GetPrimaryMonitorRect() override;

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

    /** @brief Returns the graphics debugger for this Fast3D window. */
    std::shared_ptr<GfxDebugger> GetGfxDebugger() const;

  protected:
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
    std::shared_ptr<GfxDebugger> mGfxDebugger;
};
} // namespace Fast
