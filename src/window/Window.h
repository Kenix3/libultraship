#pragma once

#include <memory>
#include <filesystem>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "graphic/Fast3D/gfx_window_manager_api.h"
#include "graphic/Fast3D/gfx_rendering_api.h"
#include "window/gui/Gui.h"

namespace LUS {
enum class WindowBackend { DX11, DX12, GLX_OPENGL, SDL_OPENGL, SDL_METAL, GX2, BACKEND_COUNT };

class Context;

class Window {
  public:
    Window();
    ~Window();

    void MainLoop(void (*mainFunction)(void));
    void Init();
    void Close();
    void StartFrame();
    void SetTargetFps(int32_t fps);
    void SetMaximumFrameLatency(int32_t latency);
    void GetPixelDepthPrepare(float x, float y);
    uint16_t GetPixelDepth(float x, float y);
    void ToggleFullscreen();
    void SetFullscreen(bool isFullscreen);
    void SetCursorVisibility(bool visible);
    uint32_t GetCurrentWidth();
    uint32_t GetCurrentHeight();
    uint32_t GetCurrentRefreshRate();
    bool CanDisableVerticalSync();
    float GetCurrentAspectRatio();
    bool IsFullscreen();
    const char* GetKeyName(int32_t scancode);
    int32_t GetLastScancode();
    void SetLastScancode(int32_t scanCode);
    void InitWindowManager();
    bool SupportsWindowedFullscreen();
    void SetResolutionMultiplier(float multiplier);
    void SetMsaaLevel(uint32_t value);
    std::shared_ptr<Gui> GetGui();
    WindowBackend GetWindowBackend();
    void SaveBackendSettingToConfig(WindowBackend backend);
    std::shared_ptr<std::vector<WindowBackend>> GetAvailableWindowBackends();

  protected:
    void SetWindowBackend(WindowBackend backend);

  private:
    static bool KeyDown(int32_t scancode);
    static bool KeyUp(int32_t scancode);
    static void AllKeysUp(void);
    static void OnFullscreenChanged(bool isNowFullscreen);

    std::shared_ptr<Gui> mGui;
    WindowBackend mWindowBackend;
    std::shared_ptr<std::vector<WindowBackend>> mAvailableWindowBackends;
    GfxRenderingAPI* mRenderingApi;
    GfxWindowManagerAPI* mWindowManagerApi;
    bool mIsFullscreen;
    uint32_t mRefreshRate;
    uint32_t mWidth;
    uint32_t mHeight;
    int32_t mLastScancode;
};
} // namespace LUS
