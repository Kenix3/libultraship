#pragma once

#include "SohConsole.h"

struct SoHConfigType {
    // Debug
    bool soh = false;
    bool n64mode = false;
    bool menu_bar = false;
    bool soh_sink = true;

    // Enhancements
    bool fast_text = false;
    bool disable_lod = false;
    bool animated_pause_menu = false;
    bool debug_mode = false;
};

extern SoHConfigType SohSettings;

namespace SohImGui {
    enum class Backend {
        DX11,
        SDL
    };

    enum class Dialogues {
        dConsole,
        dMenubar,
        dLoadSettings,
    };

    typedef struct {
        Backend backend;
        union {
            struct {
                void* window;
                void* device_context;
                void* device;
            } dx11;
            struct {
                void* window;
                void* context;
            } sdl;
        };
    } WindowImpl;

    typedef union {
        struct {
            void* handle;
            int msg;
            int wparam;
            int lparam;
        } win32;
        struct {
            void* event;
        } sdl;
    } EventImpl;

    extern Console* console;
    void Init(WindowImpl window_impl);
    void Update(EventImpl event);
    void Draw(void);
    void ShowCursor(bool hide, Dialogues w);
    std::string GetDebugSection();
    std::string GetEnhancementSection();
    void BindCmd(const std::string& cmd, CommandEntry entry);
}
