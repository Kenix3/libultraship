#pragma once

#include "gfx_window_manager_api.h"
namespace Fast {
class GfxWindowBackendSDL2 final : public GfxWindowBackend {
  public:
    GfxWindowBackendSDL2() = default;
    ~GfxWindowBackendSDL2() override;

    void Init(const char* gameName, const char* apiName, bool startFullScreen, uint32_t width, uint32_t height,
              int32_t posX, int32_t posY) override;
    void Close() override;
    void SetKeyboardCallbacks(bool (*onKeyDown)(int scancode), bool (*onKeyUp)(int scancode),
                              void (*onAllKeysUp)()) override;
    void SetMouseCallbacks(bool (*onMouseButtonDown)(int btn), bool (*onMouseButtonUp)(int btn)) override;
    void SetFullscreenChangedCallback(void (*onFullscreenChanged)(bool is_now_fullscreen)) override;
    void SetFullscreen(bool fullscreen) override;
    void GetActiveWindowRefreshRate(uint32_t* refreshRate) override;
    void SetCursorVisability(bool visability) override;
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
    void SetTargetFPS(int fps) override;
    void SetMaxFrameLatency(int latency) override;
    const char* GetKeyName(int scancode) override;
    bool CanDisableVsync() override;
    bool IsRunning() override;
    void Destroy() override;
    bool IsFullscreen() override;

  private:
    void SetFullscreenImpl(bool on, bool call_callback);
    void HandleSingleEvent(SDL_Event& event);
    int TranslateScancode(int scancode) const;
    int UntranslateScancode(int translatedScancode) const;
    void OnKeydown(int scancode) const;
    void OnKeyup(int scancode) const;
    void OnMouseButtonDown(int btn) const;
    void OnMouseButtonUp(int btn) const;
    void SyncFramerateWithTime() const;

    SDL_Window* mWnd;
    SDL_GLContext mCtx;
    SDL_Renderer* mRenderer;
    int mSdlToLusTable[512];
    float mMouseWheelX = 0.0f;
    float mMouseWheelY = 0.0f;
    // OTRTODO: These are redundant. Info can be queried from SDL.
    int mWindowWidth = 640;
    int mWindowHeight = 480;
    void (*mOnAllKeysUp)();
};
} // namespace Fast
