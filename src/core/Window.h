#pragma once

#include <memory>
#include <filesystem>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "graphic/Fast3D/gfx_window_manager_api.h"
#include "graphic/Fast3D/gfx_rendering_api.h"

namespace LUS {
class Context;

class Window {
  public:
    Window(std::shared_ptr<Context> context);
    ~Window();
    void MainLoop(void (*MainFunction)(void));
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
    void InitWindowManager(std::string windowManagerBackend, std::string gfxApiBackend);
    std::shared_ptr<Context> GetContext();

  private:
    static bool KeyDown(int32_t scancode);
    static bool KeyUp(int32_t scancode);
    static void AllKeysUp(void);
    static void OnFullscreenChanged(bool isNowFullscreen);

    std::shared_ptr<Context> mContext;
    std::string mGfxBackend;
    std::string mGfxApi;
    GfxRenderingAPI* mRenderingApi;
    GfxWindowManagerAPI* mWindowManagerApi;
    bool mIsFullscreen;
    uint32_t mRefreshRate;
    uint32_t mWidth;
    uint32_t mHeight;
    int32_t mLastScancode;
};
} // namespace LUS
