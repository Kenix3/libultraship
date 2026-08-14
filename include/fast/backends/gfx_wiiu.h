#pragma once

#ifdef __WIIU__

#include "gfx_window_manager_api.h"

#include <gx2/context.h>
#include <gx2/enum.h>

namespace Fast {

// The GX2 backend always renders into a 1080p logical framebuffer.  GX2
// scales this safely to the active TV mode and to the GamePad framebuffer.
constexpr uint32_t WIIU_DEFAULT_FB_WIDTH = 1920;
constexpr uint32_t WIIU_DEFAULT_FB_HEIGHT = 1080;

void GfxWiiUSetContextState();

class GfxWindowBackendWiiU final : public GfxWindowBackend {
  public:
    GfxWindowBackendWiiU() = default;
    ~GfxWindowBackendWiiU() override = default;

    void Init(const char* gameName, const char* apiName, bool startFullScreen, uint32_t width, uint32_t height,
              int32_t posX, int32_t posY) override;
    void Close() override;
    void SetKeyboardCallbacks(bool (*onKeyDown)(int), bool (*onKeyUp)(int), void (*onAllKeysUp)()) override;
    void SetMouseCallbacks(bool (*onMouseButtonDown)(int), bool (*onMouseButtonUp)(int)) override;
    void SetFullscreenChangedCallback(void (*onFullscreenChanged)(bool)) override;
    void SetFullscreen(bool fullscreen) override;
    void GetActiveWindowRefreshRate(uint32_t* refreshRate) override;
    void SetCursorVisibility(bool visibility) override;
    void SetMousePos(int32_t posX, int32_t posY) override;
    void GetMousePos(int32_t* x, int32_t* y) override;
    void GetMouseDelta(int32_t* x, int32_t* y) override;
    void GetMouseWheel(float* x, float* y) override;
    bool GetMouseState(uint32_t btn) override;
    void SetMouseCapture(bool capture) override;
    bool IsMouseCaptured() override;
    void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) override;
    void HandleEvents() override;
    bool IsFrameReady() override;
    void SwapBuffersBegin() override;
    void SwapBuffersEnd() override;
    double GetTime() override;
    int GetTargetFps() override;
    void SetTargetFps(int fps) override;
    void SetMaxFrameLatency(int latency) override;
    const char* GetKeyName(int scancode) override;
    bool CanDisableVsync() override;
    bool IsRunning() override;
    void Destroy() override;
    bool IsFullscreen() override;

    void SetContextState();
    bool HasForeground() const;
    void AcquireForeground();
    void ReleaseForeground();

  private:
    GX2ContextState* mContextState = nullptr;
    void* mCommandBuffer = nullptr;
    void* mMem1Storage = nullptr;
    void* mTvScanBuffer = nullptr;
    void* mDrcScanBuffer = nullptr;
    uint32_t mTvScanBufferSize = 0;
    uint32_t mDrcScanBufferSize = 0;
    GX2TVRenderMode mTvRenderMode = GX2_TV_RENDER_MODE_WIDE_720P;
    GX2DrcRenderMode mDrcRenderMode = GX2_DRC_RENDER_MODE_SINGLE;
    bool mForeground = false;
    bool mInitialized = false;
    int mFrameDivisor = 1;
    void (*mOnAllKeysUp)() = nullptr;
};

} // namespace Fast

#endif
