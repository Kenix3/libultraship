#include <stdio.h>

#if defined(ENABLE_OPENGL) || defined(__APPLE__)

#ifdef __MINGW32__
#define FOR_WINDOWS 1
#else
#define FOR_WINDOWS 0
#endif

#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/window/FileDropMgr.h"
#include "fast/backends/gfx_sdl.h"

#if FOR_WINDOWS
#include <GL/glew.h>
#include "SDL.h"
#define GL_GLEXT_PROTOTYPES 1
#include "SDL_opengl.h"
#elif __APPLE__
#include <SDL.h>
#include "fast/backends/gfx_metal.h"
#include "ship/utils/macUtils.h"
#else
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#endif

#include "ship/window/gui/Gui.h"

#ifdef _WIN32
#include <WTypesbase.h>
#include <Windows.h>
#include <SDL_syswm.h>
#endif

#define GFX_BACKEND_NAME "SDL"
#define _100NANOSECONDS_IN_SECOND 10000000

// Weak linkage for SharedGraphics API from combo library
// These functions may not be available if combo is not linked
#if defined(__GNUC__) || defined(__clang__)
extern "C" __attribute__((weak)) bool Combo_GetSharedGraphics(uint32_t* sdlWindowID, void** glContext);
extern "C" __attribute__((weak)) void Combo_SetSharedGraphics(uint32_t sdlWindowID, void* glContext);
#elif defined(_MSC_VER)
// On Windows/MSVC, we use __declspec(selectany) with function pointers
// to allow linking even if the symbols aren't present
extern "C" __declspec(dllimport) bool Combo_GetSharedGraphics(uint32_t* sdlWindowID, void** glContext);
extern "C" __declspec(dllimport) void Combo_SetSharedGraphics(uint32_t sdlWindowID, void* glContext);
#pragma comment(linker, "/alternatename:Combo_GetSharedGraphics=__Combo_GetSharedGraphics_stub")
#pragma comment(linker, "/alternatename:Combo_SetSharedGraphics=__Combo_SetSharedGraphics_stub")
extern "C" bool __Combo_GetSharedGraphics_stub(uint32_t*, void**) { return false; }
extern "C" void __Combo_SetSharedGraphics_stub(uint32_t, void*) {}
#endif

#ifdef _WIN32
LONG_PTR SDL_WndProc;
#endif

namespace Fast {
const SDL_Scancode lus_to_sdl_table[] = {
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6, /* 0 */
    SDL_SCANCODE_7,
    SDL_SCANCODE_8,
    SDL_SCANCODE_9,
    SDL_SCANCODE_0,
    SDL_SCANCODE_MINUS,
    SDL_SCANCODE_EQUALS,
    SDL_SCANCODE_BACKSPACE,
    SDL_SCANCODE_TAB, /* 0 */

    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_R,
    SDL_SCANCODE_T,
    SDL_SCANCODE_Y,
    SDL_SCANCODE_U,
    SDL_SCANCODE_I, /* 1 */
    SDL_SCANCODE_O,
    SDL_SCANCODE_P,
    SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_RIGHTBRACKET,
    SDL_SCANCODE_RETURN,
    SDL_SCANCODE_LCTRL,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S, /* 1 */

    SDL_SCANCODE_D,
    SDL_SCANCODE_F,
    SDL_SCANCODE_G,
    SDL_SCANCODE_H,
    SDL_SCANCODE_J,
    SDL_SCANCODE_K,
    SDL_SCANCODE_L,
    SDL_SCANCODE_SEMICOLON, /* 2 */
    SDL_SCANCODE_APOSTROPHE,
    SDL_SCANCODE_GRAVE,
    SDL_SCANCODE_LSHIFT,
    SDL_SCANCODE_BACKSLASH,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_X,
    SDL_SCANCODE_C,
    SDL_SCANCODE_V, /* 2 */

    SDL_SCANCODE_B,
    SDL_SCANCODE_N,
    SDL_SCANCODE_M,
    SDL_SCANCODE_COMMA,
    SDL_SCANCODE_PERIOD,
    SDL_SCANCODE_SLASH,
    SDL_SCANCODE_RSHIFT,
    SDL_SCANCODE_PRINTSCREEN, /* 3 */
    SDL_SCANCODE_LALT,
    SDL_SCANCODE_SPACE,
    SDL_SCANCODE_CAPSLOCK,
    SDL_SCANCODE_F1,
    SDL_SCANCODE_F2,
    SDL_SCANCODE_F3,
    SDL_SCANCODE_F4,
    SDL_SCANCODE_F5, /* 3 */

    SDL_SCANCODE_F6,
    SDL_SCANCODE_F7,
    SDL_SCANCODE_F8,
    SDL_SCANCODE_F9,
    SDL_SCANCODE_F10,
    SDL_SCANCODE_NUMLOCKCLEAR,
    SDL_SCANCODE_SCROLLLOCK,
    SDL_SCANCODE_HOME, /* 4 */
    SDL_SCANCODE_UP,
    SDL_SCANCODE_PAGEUP,
    SDL_SCANCODE_KP_MINUS,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_KP_5,
    SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_KP_PLUS,
    SDL_SCANCODE_END, /* 4 */

    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_PAGEDOWN,
    SDL_SCANCODE_INSERT,
    SDL_SCANCODE_DELETE,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_NONUSBACKSLASH,
    SDL_SCANCODE_F11, /* 5 */
    SDL_SCANCODE_F12,
    SDL_SCANCODE_PAUSE,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_LGUI,
    SDL_SCANCODE_RGUI,
    SDL_SCANCODE_APPLICATION,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN, /* 5 */

    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_F13,
    SDL_SCANCODE_F14,
    SDL_SCANCODE_F15,
    SDL_SCANCODE_F16, /* 6 */
    SDL_SCANCODE_F17,
    SDL_SCANCODE_F18,
    SDL_SCANCODE_F19,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN, /* 6 */

    SDL_SCANCODE_INTERNATIONAL2,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL1,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN, /* 7 */
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL4,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL5,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL3,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN /* 7 */
};

const SDL_Scancode scancode_rmapping_extended[][2] = {
    { SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_RETURN },
    { SDL_SCANCODE_RALT, SDL_SCANCODE_LALT },
    { SDL_SCANCODE_RCTRL, SDL_SCANCODE_LCTRL },
    { SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_SLASH },
    //{SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_CAPSLOCK}
};

const SDL_Scancode scancode_rmapping_nonextended[][2] = { { SDL_SCANCODE_KP_7, SDL_SCANCODE_HOME },
                                                          { SDL_SCANCODE_KP_8, SDL_SCANCODE_UP },
                                                          { SDL_SCANCODE_KP_9, SDL_SCANCODE_PAGEUP },
                                                          { SDL_SCANCODE_KP_4, SDL_SCANCODE_LEFT },
                                                          { SDL_SCANCODE_KP_6, SDL_SCANCODE_RIGHT },
                                                          { SDL_SCANCODE_KP_1, SDL_SCANCODE_END },
                                                          { SDL_SCANCODE_KP_2, SDL_SCANCODE_DOWN },
                                                          { SDL_SCANCODE_KP_3, SDL_SCANCODE_PAGEDOWN },
                                                          { SDL_SCANCODE_KP_0, SDL_SCANCODE_INSERT },
                                                          { SDL_SCANCODE_KP_PERIOD, SDL_SCANCODE_DELETE },
                                                          { SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_PRINTSCREEN } };

GfxWindowBackendSDL2::~GfxWindowBackendSDL2() {
}

void GfxWindowBackendSDL2::SetFullscreenImpl(bool on, bool call_callback) {
    if (mFullScreen == on) {
        return;
    }

    int display_in_use = SDL_GetWindowDisplayIndex(mWnd);
    if (display_in_use < 0) {
        SPDLOG_WARN("Can't detect on which monitor we are. Probably out of display area?");
        SPDLOG_WARN(SDL_GetError());
    }

    if (on) {
        // OTRTODO: Get mode from config.
        SDL_DisplayMode mode;
        if (SDL_GetDesktopDisplayMode(display_in_use, &mode) >= 0) {
            SDL_SetWindowDisplayMode(mWnd, &mode);
        } else {
            SPDLOG_ERROR(SDL_GetError());
        }
    }

#if defined(__APPLE__)
    // Implement fullscreening with native macOS APIs
    if (on != isNativeMacOSFullscreenActive(mWnd)) {
        toggleNativeMacOSFullscreen(mWnd);
    }
    mFullScreen = on;
#else
    if (SDL_SetWindowFullscreen(
            mWnd, on ? (Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_SDL_WINDOWED_FULLSCREEN, 0)
                            ? SDL_WINDOW_FULLSCREEN_DESKTOP
                            : SDL_WINDOW_FULLSCREEN)
                     : 0) >= 0) {
        mFullScreen = on;
    } else {
        SPDLOG_ERROR("Failed to switch from or to fullscreen mode.");
        SPDLOG_ERROR(SDL_GetError());
    }
#endif

    if (!on) {
        auto conf = Ship::Context::GetInstance()->GetConfig();
        mWindowWidth = conf->GetInt("Window.Width", 640);
        mWindowHeight = conf->GetInt("Window.Height", 480);
        int32_t posX = conf->GetInt("Window.PositionX", 100);
        int32_t posY = conf->GetInt("Window.PositionY", 100);
        if (display_in_use < 0) { // Fallback to default if out of bounds
            posX = 100;
            posY = 100;
        }
        SDL_SetWindowPosition(mWnd, posX, posY);
        SDL_SetWindowSize(mWnd, mWindowWidth, mWindowHeight);
    }

    if (mOnFullscreenChanged != nullptr && call_callback) {
        mOnFullscreenChanged(on);
    }
}

void GfxWindowBackendSDL2::GetActiveWindowRefreshRate(uint32_t* refresh_rate) {
    int display_in_use = SDL_GetWindowDisplayIndex(mWnd);

    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(display_in_use, &mode);
    *refresh_rate = mode.refresh_rate != 0 ? mode.refresh_rate : 60;
}

static uint64_t previous_time;
#ifdef _WIN32
static HANDLE mTimer;
#endif

#define FRAME_INTERVAL_US_NUMERATOR 1000000
#define FRAME_INTERVAL_US_DENOMINATOR (mTargetFps)

void GfxWindowBackendSDL2::Close() {
    mIsRunning = false;
}

#ifdef _WIN32
static LRESULT CALLBACK gfx_sdl_wnd_proc(HWND h_wnd, UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
        case WM_GETDPISCALEDSIZE:
            // Something is wrong with SDLs original implementation of WM_GETDPISCALEDSIZE, so pass it to the default
            // system window procedure instead.
            return DefWindowProc(h_wnd, message, w_param, l_param);
        case WM_ENDSESSION: {
            GfxWindowBackendSDL2* self =
                reinterpret_cast<GfxWindowBackendSDL2*>(GetWindowLongPtr(h_wnd, GWLP_USERDATA));
            // Apparently SDL2 does not handle this
            if (w_param == TRUE) {
                self->Close();
            }
            break;
        }
        default:
            // Pass anything else to SDLs original window procedure.
            return CallWindowProc((WNDPROC)SDL_WndProc, h_wnd, message, w_param, l_param);
    }
    return 0;
};
#endif

void GfxWindowBackendSDL2::Init(const char* gameName, const char* gfxApiName, bool startFullScreen, uint32_t width,
                                uint32_t height, int32_t posX, int32_t posY) {
    mWindowWidth = width;
    mWindowHeight = height;

#if SDL_VERSION_ATLEAST(2, 24, 0)
    /* fix DPI scaling issues on Windows */
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

    SDL_Init(SDL_INIT_VIDEO);

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

#if defined(__APPLE__)
    bool use_opengl = strcmp(gfxApiName, "OpenGL") == 0;
#else
    constexpr bool use_opengl = true;
#endif

    if (use_opengl) {
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    } else {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    }

#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

#ifdef _WIN32
    // Use high-resolution mTimer by default on Windows 10 (so that NtSetTimerResolution (...) hacks are not needed)
    mTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    // Fallback to low resolution mTimer if unsupported by the OS
    if (mTimer == nullptr) {
        mTimer = CreateWaitableTimer(nullptr, false, nullptr);
    }
#endif

    char title[512];
    int len = sprintf(title, "%s (%s)", gameName, gfxApiName);

#ifdef __IOS__
    Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN;
#else
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    if (use_opengl) {
        flags = flags | SDL_WINDOW_OPENGL;
    } else {
        flags = flags | SDL_WINDOW_METAL;
    }

    Ship::GuiWindowInitData window_impl;

    // Check for shared graphics from combo library (for game switching)
    uint32_t sharedWindowID = 0;
    void* sharedGLContext = nullptr;
    bool hasSharedGraphics = false;

#if defined(__GNUC__) || defined(__clang__)
    // Weak symbol will be nullptr if combo is not linked
    if (Combo_GetSharedGraphics != nullptr) {
        hasSharedGraphics = Combo_GetSharedGraphics(&sharedWindowID, &sharedGLContext);
    }
#else
    // On MSVC, the stub functions handle the case where combo isn't linked
    hasSharedGraphics = Combo_GetSharedGraphics(&sharedWindowID, &sharedGLContext);
#endif

    if (hasSharedGraphics && sharedWindowID != 0 && sharedGLContext != nullptr && use_opengl) {
        // Reuse existing window and GL context from another game
        mWnd = SDL_GetWindowFromID(sharedWindowID);
        mCtx = static_cast<SDL_GLContext>(sharedGLContext);
        mUsingSharedGraphics = true;

        if (mWnd != nullptr) {
            SDL_GL_GetDrawableSize(mWnd, &mWindowWidth, &mWindowHeight);
            SDL_GL_MakeCurrent(mWnd, mCtx);
            SDL_GL_SetSwapInterval(mVsyncEnabled ? 1 : 0);

            window_impl.Opengl = { mWnd, mCtx };
            Ship::Context::GetInstance()->GetWindow()->GetGui()->Init(window_impl);

            // Initialize scancode tables and return early
            for (size_t i = 0; i < std::size(lus_to_sdl_table); i++) {
                mSdlToLusTable[lus_to_sdl_table[i]] = i;
            }
            for (size_t i = 0; i < std::size(scancode_rmapping_extended); i++) {
                mSdlToLusTable[scancode_rmapping_extended[i][0]] = mSdlToLusTable[scancode_rmapping_extended[i][1]] + 0x100;
            }
            for (size_t i = 0; i < std::size(scancode_rmapping_nonextended); i++) {
                mSdlToLusTable[scancode_rmapping_nonextended[i][0]] = mSdlToLusTable[scancode_rmapping_nonextended[i][1]];
                mSdlToLusTable[scancode_rmapping_nonextended[i][1]] += 0x100;
            }
            return;
        }
        // If SDL_GetWindowFromID failed, fall through to normal window creation
        mUsingSharedGraphics = false;
    }

    // Normal path: create new window
    mUsingSharedGraphics = false;
    mWnd = SDL_CreateWindow(title, posX, posY, mWindowWidth, mWindowHeight, flags);
#ifdef _WIN32
    // Get Windows window handle and use it to subclass the window procedure.
    // Needed to circumvent SDLs DPI scaling problems under windows (original does only scale *sometimes*).
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(mWnd, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;
    SDL_WndProc = SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)gfx_sdl_wnd_proc);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
#endif

    int display_in_use = SDL_GetWindowDisplayIndex(mWnd);
    if (display_in_use < 0) { // Fallback to default if out of bounds
        posX = 100;
        posY = 100;
    }

    if (use_opengl) {
        SDL_GL_GetDrawableSize(mWnd, &mWindowWidth, &mWindowHeight);

        if (startFullScreen) {
            SetFullscreenImpl(true, false);
        }

        mCtx = SDL_GL_CreateContext(mWnd);

        SDL_GL_MakeCurrent(mWnd, mCtx);
        SDL_GL_SetSwapInterval(mVsyncEnabled ? 1 : 0);

        window_impl.Opengl = { mWnd, mCtx };

        // Store window/context for other games to reuse
#if defined(__GNUC__) || defined(__clang__)
        if (Combo_SetSharedGraphics != nullptr) {
            Combo_SetSharedGraphics(SDL_GetWindowID(mWnd), mCtx);
        }
#else
        Combo_SetSharedGraphics(SDL_GetWindowID(mWnd), mCtx);
#endif
    } else {
        uint32_t flags = SDL_RENDERER_ACCELERATED;
        if (mVsyncEnabled) {
            flags |= SDL_RENDERER_PRESENTVSYNC;
        }
        mRenderer = SDL_CreateRenderer(mWnd, -1, flags);
        if (mRenderer == nullptr) {
            SPDLOG_ERROR("Error creating renderer: {}", SDL_GetError());
            return;
        }

        if (startFullScreen) {
            SetFullscreenImpl(true, false);
        }

        SDL_GetRendererOutputSize(mRenderer, &mWindowWidth, &mWindowHeight);
        window_impl.Metal = { mWnd, mRenderer };
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->Init(window_impl);

    for (size_t i = 0; i < std::size(lus_to_sdl_table); i++) {
        mSdlToLusTable[lus_to_sdl_table[i]] = i;
    }

    for (size_t i = 0; i < std::size(scancode_rmapping_extended); i++) {
        mSdlToLusTable[scancode_rmapping_extended[i][0]] = mSdlToLusTable[scancode_rmapping_extended[i][1]] + 0x100;
    }

    for (size_t i = 0; i < std::size(scancode_rmapping_nonextended); i++) {
        mSdlToLusTable[scancode_rmapping_nonextended[i][0]] = mSdlToLusTable[scancode_rmapping_nonextended[i][1]];
        mSdlToLusTable[scancode_rmapping_nonextended[i][1]] += 0x100;
    }
}

void GfxWindowBackendSDL2::SetFullscreenChangedCallback(void (*onFullscreenChanged)(bool is_now_fullscreen)) {
    mOnFullscreenChanged = onFullscreenChanged;
}

void GfxWindowBackendSDL2::SetFullscreen(bool enable) {
    SetFullscreenImpl(enable, true);
}

void GfxWindowBackendSDL2::SetCursorVisibility(bool visible) {
    if (visible) {
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }
}

void GfxWindowBackendSDL2::SetMousePos(int32_t x, int32_t y) {
    SDL_WarpMouseInWindow(mWnd, x, y);
}

void GfxWindowBackendSDL2::GetMousePos(int32_t* x, int32_t* y) {
    SDL_GetMouseState(x, y);
}

void GfxWindowBackendSDL2::GetMouseDelta(int32_t* x, int32_t* y) {
    SDL_GetRelativeMouseState(x, y);
}

void GfxWindowBackendSDL2::GetMouseWheel(float* x, float* y) {
    *x = mMouseWheelX;
    *y = mMouseWheelY;
    mMouseWheelX = 0.0f;
    mMouseWheelY = 0.0f;
}

bool GfxWindowBackendSDL2::GetMouseState(uint32_t btn) {
    return SDL_GetMouseState(nullptr, nullptr) & (1 << btn);
}

void GfxWindowBackendSDL2::SetMouseCapture(bool capture) {
    SDL_SetRelativeMouseMode(static_cast<SDL_bool>(capture));
    // TODO: Manually setting a clipping rect here because
    // https://wiki.libsdl.org/SDL2/SDL_HINT_MOUSE_RELATIVE_MODE_CENTER isn't working as epxected.
    // Revisit on SDL3
    auto mouse = SDL_GetWindowMouseRect(mWnd);
    if (capture) {
        int w, h;
        SDL_GetWindowSize(mWnd, &w, &h);
        mCursorClip = { (w / 2) - 1, (h / 2) - 1, 2, 2 };
    }
    SDL_SetWindowMouseRect(mWnd, capture ? &mCursorClip : NULL);
}

bool GfxWindowBackendSDL2::IsMouseCaptured() {
    return (SDL_GetRelativeMouseMode() == SDL_TRUE);
}

void GfxWindowBackendSDL2::SetKeyboardCallbacks(bool (*onKeyDown)(int scancode), bool (*onKeyUp)(int scancode),
                                                void (*onAllKeysUp)()) {
    mOnKeyDown = onKeyDown;
    mOnKeyUp = onKeyUp;
    mOnAllKeysUp = onAllKeysUp;
}

void GfxWindowBackendSDL2::SetMouseCallbacks(bool (*onMouseButtonDown)(int btn), bool (*onMouseButtonUp)(int btn)) {
    mOnMouseButtonDown = onMouseButtonDown;
    mOnMouseButtonUp = onMouseButtonUp;
}

void GfxWindowBackendSDL2::GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) {
#ifdef __APPLE__
    SDL_GetWindowSize(mWnd, static_cast<int*>((void*)width), static_cast<int*>((void*)height));
#else
    SDL_GL_GetDrawableSize(mWnd, static_cast<int*>((void*)width), static_cast<int*>((void*)height));
#endif
    SDL_GetWindowPosition(mWnd, static_cast<int*>(posX), static_cast<int*>(posY));
}

int GfxWindowBackendSDL2::TranslateScancode(int scancode) const {
    if (scancode < 512) {
        return mSdlToLusTable[scancode];
    }
    return 0;
}

int GfxWindowBackendSDL2::UntranslateScancode(int translatedScancode) const {
    for (int i = 0; i < 512; i++) {
        if (mSdlToLusTable[i] == translatedScancode) {
            return i;
        }
    }

    return 0;
}

void GfxWindowBackendSDL2::OnKeydown(int scancode) const {
    int key = TranslateScancode(scancode);
    if (mOnKeyDown != nullptr) {
        mOnKeyDown(key);
    }
}

void GfxWindowBackendSDL2::OnKeyup(int scancode) const {
    int key = TranslateScancode(scancode);
    if (mOnKeyUp != nullptr) {
        mOnKeyUp(key);
    }
}

void GfxWindowBackendSDL2::OnMouseButtonDown(int btn) const {
    if (!(btn >= 0 && btn < 5)) {
        return;
    }
    if (mOnMouseButtonDown != nullptr) {
        mOnMouseButtonDown(btn);
    }
}

void GfxWindowBackendSDL2::OnMouseButtonUp(int btn) const {
    if (mOnMouseButtonUp != nullptr) {
        mOnMouseButtonUp(btn);
    }
}

void GfxWindowBackendSDL2::HandleSingleEvent(SDL_Event& event) {
    Ship::WindowEvent event_impl;
    event_impl.Sdl = { &event };
    Ship::Context::GetInstance()->GetWindow()->GetGui()->HandleWindowEvents(event_impl);
    switch (event.type) {
#ifndef TARGET_WEB
        // Scancodes are broken in Emscripten SDL2: https://bugzilla.libsdl.org/show_bug.cgi?id=3259
        case SDL_KEYDOWN:
            OnKeydown(event.key.keysym.scancode);
            break;
        case SDL_KEYUP:
            OnKeyup(event.key.keysym.scancode);
            break;
        case SDL_MOUSEBUTTONDOWN:
            OnMouseButtonDown(event.button.button - 1);
            break;
        case SDL_MOUSEBUTTONUP:
            OnMouseButtonUp(event.button.button - 1);
            break;
        case SDL_MOUSEWHEEL:
            mMouseWheelX = event.wheel.x;
            mMouseWheelY = event.wheel.y;
            break;
#endif
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
#ifdef __APPLE__
                    SDL_GetWindowSize(mWnd, &mWindowWidth, &mWindowHeight);
#else
                    SDL_GL_GetDrawableSize(mWnd, &mWindowWidth, &mWindowHeight);
#endif
                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    if (event.window.windowID == SDL_GetWindowID(mWnd)) {
                        // We listen specifically for main window close because closing main window
                        // on macOS does not trigger SDL_Quit.
                        Close();
                    }
                    break;
            }
            break;
        case SDL_DROPFILE:
            Ship::Context::GetInstance()->GetFileDropMgr()->SetDroppedFile(event.drop.file);
            break;
        case SDL_QUIT:
            Close();
            break;
    }
}

void GfxWindowBackendSDL2::HandleEvents() {
    SDL_Event event;
    SDL_PumpEvents();
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_CONTROLLERDEVICEADDED - 1) > 0) {
        HandleSingleEvent(event);
    }
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEREMOVED + 1, SDL_LASTEVENT) > 0) {
        HandleSingleEvent(event);
    }

    // resync fullscreen state
#ifdef __APPLE__
    auto nextFullscreenState = isNativeMacOSFullscreenActive(mWnd);
    if (mFullScreen != nextFullscreenState) {
        mFullScreen = nextFullscreenState;
        if (mOnFullscreenChanged != nullptr) {
            mOnFullscreenChanged(mFullScreen);
        }
    }
#endif
}

bool GfxWindowBackendSDL2::IsFrameReady() {
    return true;
}

static uint64_t qpc_to_100ns(uint64_t qpc) {
    const uint64_t qpc_freq = SDL_GetPerformanceFrequency();
    return qpc / qpc_freq * _100NANOSECONDS_IN_SECOND + qpc % qpc_freq * _100NANOSECONDS_IN_SECOND / qpc_freq;
}

void GfxWindowBackendSDL2::SyncFramerateWithTime() const {
    uint64_t t = qpc_to_100ns(SDL_GetPerformanceCounter());

    const int64_t next = previous_time + 10 * FRAME_INTERVAL_US_NUMERATOR / FRAME_INTERVAL_US_DENOMINATOR;
    int64_t left = next - t;
#ifdef _WIN32
    // We want to exit a bit early, so we can busy-wait the rest to never miss the deadline
    left -= 15000UL;
#elif defined(__APPLE__)
    // Use macOS scheduler interval on macOS
    left -= 10000UL;
#endif
    if (left > 0) {
#ifndef _WIN32
        const timespec spec = { 0, left * 100 };
        nanosleep(&spec, nullptr);
#else
        // The accuracy of this mTimer seems to usually be within +- 1.0 ms
        LARGE_INTEGER li;
        li.QuadPart = -left;
        SetWaitableTimer(mTimer, &li, 0, nullptr, nullptr, false);
        WaitForSingleObject(mTimer, INFINITE);
#endif
    }

#ifdef _WIN32
    t = qpc_to_100ns(SDL_GetPerformanceCounter());
    while (t < next) {
        YieldProcessor(); // TODO: Find a way for other compilers, OSes and architectures
        t = qpc_to_100ns(SDL_GetPerformanceCounter());
    }
#endif
    t = qpc_to_100ns(SDL_GetPerformanceCounter());
    if (left > 0 && t - next < 10000) {
        // In case it takes some time for the application to wake up after sleep,
        // or inaccurate mTimer,
        // don't let that slow down the framerate.
        t = next;
    }
    previous_time = t;
}

void GfxWindowBackendSDL2::SwapBuffersBegin() {
    bool nextVsyncEnabled = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_VSYNC_ENABLED, 1);

    if (mVsyncEnabled != nextVsyncEnabled) {
        mVsyncEnabled = nextVsyncEnabled;
        SDL_GL_SetSwapInterval(mVsyncEnabled ? 1 : 0);
        SDL_RenderSetVSync(mRenderer, mVsyncEnabled ? 1 : 0);
    }

    SyncFramerateWithTime();
    SDL_GL_SwapWindow(mWnd);
}

void GfxWindowBackendSDL2::SwapBuffersEnd() {
}

double GfxWindowBackendSDL2::GetTime() {
    return 0.0;
}

int GfxWindowBackendSDL2::GetTargetFps() {
    return mTargetFps;
}

void GfxWindowBackendSDL2::SetTargetFps(int fps) {
    mTargetFps = fps;
}

void GfxWindowBackendSDL2::SetMaxFrameLatency(int latency) {
    // Not supported by SDL :(
}

const char* GfxWindowBackendSDL2::GetKeyName(int scancode) {
    return SDL_GetScancodeName((SDL_Scancode)UntranslateScancode(scancode));
}

bool GfxWindowBackendSDL2::CanDisableVsync() {
    return true;
}

bool GfxWindowBackendSDL2::IsRunning() {
    return mIsRunning;
}

void GfxWindowBackendSDL2::Destroy() {
    // TODO: destroy _any_ resources used by SDL
    if (!mUsingSharedGraphics) {
        // Only destroy window/context if we own them
        // When using shared graphics, leave them alive for other games
        SDL_GL_DeleteContext(mCtx);
        SDL_DestroyWindow(mWnd);
        SDL_DestroyRenderer(mRenderer);
        SDL_Quit();
    }
    // If using shared graphics, don't destroy anything - other games need them
}

bool GfxWindowBackendSDL2::IsFullscreen() {
    return mFullScreen;
}
} // namespace Fast
#endif
