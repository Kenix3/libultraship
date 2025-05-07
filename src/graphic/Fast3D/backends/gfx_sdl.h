#ifndef GFX_SDL_H
#define GFX_SDL_H

#include "gfx_window_manager_api.h"

class GfxBackendSDL2 final : public GfxBackend {
public:
    GfxBackendSDL2() = default;
    ~GfxBackendSDL2() override;

    void Init(const char* gameName, const char* apiName, bool startFullScreen, uint32_t width, uint32_t height, int32_t posX, int32_t posY) override;
    void Close() override;
    void SetKeyboardCallbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode),
                                     void (*on_all_keys_up)()) override;
    void SetMouseCallbacks(bool (*on_mouse_button_down)(int btn), bool (*on_mouse_button_up)(int btn)) override;
    void SetFullscreenChangedCallback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) override;
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
    int mTargetFps = 60;
    bool mVsyncEnabled = true;
    float mMouseWheelX = 0.0f;
    float mMouseWheelY = 0.0f;
    // OTRTODO: These are redundant. Info can be queried from SDL.
    int mWindowWidth = 640;
    int mWindowHeight = 480;
    bool mFullScreen;
    bool mIsRunning = true;
    void (*mOnFullscreenChangedCallback)(bool is_now_fullscreen);
    bool (*mOnKeyDownCallback)(int scancode);
    bool (*mOnKeyUpCallback)(int scancode);
    void (*mOnAllKeysUpCallback)();
    bool (*mOnMouseButtonDownCallback)(int btn);
    bool (*mOnMouseButtonUpCallback)(int btn);
};

#endif
