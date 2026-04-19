#ifdef ENABLE_VULKAN
#pragma once

#include "gfx_window_manager_api.h"

#include <SDL2/SDL.h>

namespace Fast {

/**
 * @brief SDL2-based window backend for the Vulkan rendering API.
 *
 * Creates an SDL window with the SDL_WINDOW_VULKAN flag so that
 * GfxRenderingAPIVulkan can create a VkSurfaceKHR from it.
 * Buffer-swap (presentation) is driven by the rendering API through
 * the Vulkan swapchain, so SwapBuffersBegin/End are no-ops here.
 */
class GfxWindowBackendSDLVulkan final : public GfxWindowBackend {
  public:
    GfxWindowBackendSDLVulkan() = default;
    ~GfxWindowBackendSDLVulkan() override;

    void Init(const char* gameName, const char* apiName, bool startFullScreen, uint32_t width, uint32_t height,
              int32_t posX, int32_t posY) override;
    void Close() override;
    void SetKeyboardCallbacks(bool (*onKeyDown)(int scancode), bool (*onKeyUp)(int scancode),
                              void (*onAllKeysUp)()) override;
    void SetMouseCallbacks(bool (*onMouseButtonDown)(int btn), bool (*onMouseButtonUp)(int btn)) override;
    void SetFullscreenChangedCallback(void (*onFullscreenChanged)(bool is_now_fullscreen)) override;
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

    SDL_Window* GetSDLWindow() const { return mWnd; }

  private:
    void SetFullscreenImpl(bool on, bool callCallback);
    void HandleSingleEvent(SDL_Event& event);

    SDL_Window* mWnd = nullptr;
    int mWindowWidth = 640;
    int mWindowHeight = 480;
    int mSdlToLusTable[512] = {};
    float mMouseWheelX = 0.0f;
    float mMouseWheelY = 0.0f;
    void (*mOnAllKeysUp)() = nullptr;
};

} // namespace Fast
#endif // ENABLE_VULKAN
