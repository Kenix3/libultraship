#ifndef GFX_WINDOW_MANAGER_API_H
#define GFX_WINDOW_MANAGER_API_H

#include <stdint.h>
#include <stdbool.h>

class GfxWindowBackend {
  public:
    virtual ~GfxWindowBackend() = default;
    virtual void Init(const char* gameName, const char* apiName, bool startFullScreen, uint32_t width, uint32_t height,
                      int32_t posX, int32_t posY) = 0;
    virtual void Close() = 0;
    virtual void SetKeyboardCallbacks(bool (*mOnKeyDown)(int scancode), bool (*mOnKeyUp)(int scancode),
                                      void (*mOnAllKeysUp)()) = 0;
    virtual void SetMouseCallbacks(bool (*mOnMouseButtonDown)(int btn), bool (*mOnMouseButtonUp)(int btn)) = 0;
    virtual void SetFullscreenChangedCallback(void (*mOnFullscreenChanged)(bool is_now_fullscreen)) = 0;
    virtual void SetFullscreen(bool fullscreen) = 0;
    virtual void GetActiveWindowRefreshRate(uint32_t* refreshRate) = 0;
    virtual void SetCursorVisability(bool visability) = 0;
    virtual void SetMousePos(int32_t posX, int32_t posY) = 0;
    virtual void GetMousePos(int32_t* x, int32_t* y) = 0;
    virtual void GetMouseDelta(int32_t* x, int32_t* y) = 0;
    virtual void GetMouseWheel(float* x, float* y) = 0;
    virtual bool GetMouseState(uint32_t btn) = 0;
    virtual void SetMouseCapture(bool capture) = 0;
    virtual bool IsMouseCaptured() = 0;
    virtual void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) = 0;
    virtual void HandleEvents() = 0;
    virtual bool IsFrameReady() = 0;
    virtual void SwapBuffersBegin() = 0;
    virtual void SwapBuffersEnd() = 0;
    virtual double GetTime() = 0;
    virtual void SetTargetFPS(int fps) = 0;
    virtual void SetMaxFrameLatency(int latency) = 0;
    virtual const char* GetKeyName(int scancode) = 0;
    virtual bool CanDisableVsync() = 0;
    virtual bool IsRunning() = 0;
    virtual void Destroy() = 0;
    virtual bool IsFullscreen() = 0;

  protected:
    void (*mOnFullscreenChanged)(bool isNowFullscreen);
    bool (*mOnKeyDown)(int scancode);
    bool (*mOnKeyUp)(int scancode);
    bool (*mOnMouseButtonDown)(int btn);
    bool (*mOnMouseButtonUp)(int btn);
    uint32_t mTargetFps = 60;
    bool mFullScreen;
    bool mIsRunning = true;
    bool mVsyncEnabled = true;
};

#if 0

struct GfxWindowBackend {
    void (*init)(const char* game_name, const char* gfx_api_name, bool start_in_fullscreen, uint32_t width,
                 uint32_t height, int32_t posX, int32_t posY) = 0;
    void (*close)() = 0;
    void (*set_keyboard_callbacks)(bool (*mOnKeyDown)(int scancode), bool (*mOnKeyUp)(int scancode),
                                   void (*mOnAllKeysUp)());
    void (*set_mouse_callbacks)(bool (*mOnMouseButtonDown)(int btn), bool (*mOnMouseButtonUp)(int btn));
    void (*set_fullscreen_changed_callback)(void (*on_fullscreen_changed)(bool is_now_fullscreen));
    void (*set_fullscreen)(bool enable) = 0;
    void (*get_active_window_refresh_rate)(uint32_t* refresh_rate) = 0;
    void (*set_cursor_visibility)(bool visible) = 0;
    void (*set_mouse_pos)(int32_t x, int32_t y) = 0;
    void (*get_mouse_pos)(int32_t* x, int32_t* y) = 0;
    void (*get_mouse_delta)(int32_t* x, int32_t* y) = 0;
    void (*get_mouse_wheel)(float* x, float* y) = 0;
    bool (*get_mouse_state)(uint32_t btn) = 0;
    void (*set_mouse_capture)(bool capture) = 0;
    bool (*mIsMouseCaptured)() = 0;
    void (*get_dimensions)(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) = 0;
    void (*handle_events)() = 0;
    bool (*is_frame_ready)() = 0;
    void (*swap_buffers_begin)() = 0;
    void (*swap_buffers_end)() = 0;
    double (*get_time)() = 0; // For debug
    void (*set_target_fps)(int fps) = 0;
    void (*set_maximum_frame_latency)(int latency) = 0;
    const char* (*get_key_name)(int scancode) = 0;
    bool (*can_disable_vsync)() = 0;
    bool (*mIsRunning)() = 0;
    void (*destroy)() = 0;
    bool (*is_fullscreen)() = 0;
};
#endif
#endif
