#ifndef GFX_WINDOW_MANAGER_API_H
#define GFX_WINDOW_MANAGER_API_H

#include <stdint.h>
#include <stdbool.h>

struct GfxWindowManagerAPI {
    void (*init)(const char* game_name, const char* gfx_api_name, bool start_in_fullscreen, uint32_t width,
                 uint32_t height, int32_t posX, int32_t posY);
    void (*close)();
    void (*set_keyboard_callbacks)(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode),
                                   void (*on_all_keys_up)());
    void (*set_fullscreen_changed_callback)(void (*on_fullscreen_changed)(bool is_now_fullscreen));
    void (*set_fullscreen)(bool enable);
    void (*get_active_window_refresh_rate)(uint32_t* refresh_rate);
    void (*set_cursor_visibility)(bool visible);
    void (*set_mouse_pos)(int32_t x, int32_t y);
    void (*get_mouse_pos)(int32_t* x, int32_t* y);
    void (*get_mouse_delta)(int32_t* x, int32_t* y);
    void (*get_mouse_wheel)(float* x, float* y);
    bool (*get_mouse_state)(uint32_t btn);
    void (*set_mouse_capture)(bool capture);
    bool (*is_mouse_captured)();
    void (*get_dimensions)(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY);
    void (*handle_events)();
    bool (*start_frame)();
    void (*swap_buffers_begin)();
    void (*swap_buffers_end)();
    double (*get_time)(); // For debug
    void (*set_target_fps)(int fps);
    void (*set_maximum_frame_latency)(int latency);
    const char* (*get_key_name)(int scancode);
    bool (*can_disable_vsync)();
    bool (*is_running)();
    void (*destroy)();
    bool (*is_fullscreen)();
};

#endif
