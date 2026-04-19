#ifdef ENABLE_VULKAN

#include "fast/backends/gfx_sdl_vulkan.h"

#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/window/gui/Gui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <spdlog/spdlog.h>

// Re-use the SDL scancode translation tables from gfx_sdl2.cpp via an extern
// declaration.  They are defined as file-static there, so we duplicate the
// minimal lookup logic here instead.
namespace Fast {

// ---- scancode tables (identical to those in gfx_sdl2.cpp) ------------------
static const SDL_Scancode lus_to_sdl_table_vk[] = {
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_ESCAPE, SDL_SCANCODE_1,        SDL_SCANCODE_2,
    SDL_SCANCODE_3,         SDL_SCANCODE_4,      SDL_SCANCODE_5,        SDL_SCANCODE_6,
    SDL_SCANCODE_7,         SDL_SCANCODE_8,      SDL_SCANCODE_9,        SDL_SCANCODE_0,
    SDL_SCANCODE_MINUS,     SDL_SCANCODE_EQUALS, SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_TAB,
    SDL_SCANCODE_Q,         SDL_SCANCODE_W,      SDL_SCANCODE_E,        SDL_SCANCODE_R,
    SDL_SCANCODE_T,         SDL_SCANCODE_Y,      SDL_SCANCODE_U,        SDL_SCANCODE_I,
    SDL_SCANCODE_O,         SDL_SCANCODE_P,      SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_RIGHTBRACKET,
    SDL_SCANCODE_RETURN,    SDL_SCANCODE_LCTRL,  SDL_SCANCODE_A,        SDL_SCANCODE_S,
    SDL_SCANCODE_D,         SDL_SCANCODE_F,      SDL_SCANCODE_G,        SDL_SCANCODE_H,
    SDL_SCANCODE_J,         SDL_SCANCODE_K,      SDL_SCANCODE_L,        SDL_SCANCODE_SEMICOLON,
    SDL_SCANCODE_APOSTROPHE, SDL_SCANCODE_GRAVE, SDL_SCANCODE_LSHIFT,   SDL_SCANCODE_BACKSLASH,
    SDL_SCANCODE_Z,         SDL_SCANCODE_X,      SDL_SCANCODE_C,        SDL_SCANCODE_V,
    SDL_SCANCODE_B,         SDL_SCANCODE_N,      SDL_SCANCODE_M,        SDL_SCANCODE_COMMA,
    SDL_SCANCODE_PERIOD,    SDL_SCANCODE_SLASH,  SDL_SCANCODE_RSHIFT,   SDL_SCANCODE_PRINTSCREEN,
    SDL_SCANCODE_LALT,      SDL_SCANCODE_SPACE,  SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_F1,
    SDL_SCANCODE_F2,        SDL_SCANCODE_F3,     SDL_SCANCODE_F4,       SDL_SCANCODE_F5,
    SDL_SCANCODE_F6,        SDL_SCANCODE_F7,     SDL_SCANCODE_F8,       SDL_SCANCODE_F9,
    SDL_SCANCODE_F10,       SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_SCROLLLOCK, SDL_SCANCODE_HOME,
    SDL_SCANCODE_UP,        SDL_SCANCODE_PAGEUP, SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_LEFT,
    SDL_SCANCODE_KP_5,      SDL_SCANCODE_RIGHT,  SDL_SCANCODE_KP_PLUS,  SDL_SCANCODE_END,
    SDL_SCANCODE_DOWN,      SDL_SCANCODE_PAGEDOWN, SDL_SCANCODE_INSERT, SDL_SCANCODE_DELETE,
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_NONUSBACKSLASH, SDL_SCANCODE_F11,
    SDL_SCANCODE_F12,       SDL_SCANCODE_PAUSE,  SDL_SCANCODE_UNKNOWN,  SDL_SCANCODE_LGUI,
    SDL_SCANCODE_RGUI,      SDL_SCANCODE_APPLICATION, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
};

GfxWindowBackendSDLVulkan::~GfxWindowBackendSDLVulkan() = default;

void GfxWindowBackendSDLVulkan::SetFullscreenImpl(bool on, bool callCallback) {
    if (mFullScreen == on) {
        return;
    }
    mFullScreen = on;
    SDL_SetWindowFullscreen(mWnd, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    if (callCallback && mOnFullscreenChanged) {
        mOnFullscreenChanged(on);
    }
}

void GfxWindowBackendSDLVulkan::Init(const char* gameName, const char* apiName, bool startFullScreen,
                                     uint32_t width, uint32_t height, int32_t posX, int32_t posY) {
    mWindowWidth = static_cast<int>(width);
    mWindowHeight = static_cast<int>(height);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    char title[512];
    snprintf(title, sizeof(title), "%s (%s)", gameName, apiName);

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN;
    mWnd = SDL_CreateWindow(title, posX, posY, mWindowWidth, mWindowHeight, flags);
    if (mWnd == nullptr) {
        SPDLOG_CRITICAL("Failed to create SDL Vulkan window: {}", SDL_GetError());
        return;
    }

    if (startFullScreen) {
        SetFullscreenImpl(true, false);
    }

    // Build LUS→SDL scancode lookup
    for (size_t i = 0; i < std::size(lus_to_sdl_table_vk); i++) {
        if (lus_to_sdl_table_vk[i] != SDL_SCANCODE_UNKNOWN) {
            mSdlToLusTable[lus_to_sdl_table_vk[i]] = static_cast<int>(i);
        }
    }

    Ship::GuiWindowInitData windowImpl;
    windowImpl.Vulkan = { mWnd };
    Ship::Context::GetInstance()->GetWindow()->GetGui()->Init(windowImpl);
}

void GfxWindowBackendSDLVulkan::Close() {
    mIsRunning = false;
}

void GfxWindowBackendSDLVulkan::SetKeyboardCallbacks(bool (*onKeyDown)(int), bool (*onKeyUp)(int),
                                                      void (*onAllKeysUp)()) {
    mOnKeyDown = onKeyDown;
    mOnKeyUp = onKeyUp;
    mOnAllKeysUp = onAllKeysUp;
}

void GfxWindowBackendSDLVulkan::SetMouseCallbacks(bool (*onMouseButtonDown)(int), bool (*onMouseButtonUp)(int)) {
    mOnMouseButtonDown = onMouseButtonDown;
    mOnMouseButtonUp = onMouseButtonUp;
}

void GfxWindowBackendSDLVulkan::SetFullscreenChangedCallback(void (*cb)(bool)) {
    mOnFullscreenChanged = cb;
}

void GfxWindowBackendSDLVulkan::SetFullscreen(bool fullscreen) {
    SetFullscreenImpl(fullscreen, true);
}

void GfxWindowBackendSDLVulkan::GetActiveWindowRefreshRate(uint32_t* refreshRate) {
    int displayIndex = SDL_GetWindowDisplayIndex(mWnd);
    SDL_DisplayMode mode;
    if (displayIndex >= 0 && SDL_GetCurrentDisplayMode(displayIndex, &mode) == 0 && mode.refresh_rate > 0) {
        *refreshRate = static_cast<uint32_t>(mode.refresh_rate);
    } else {
        *refreshRate = 60;
    }
}

void GfxWindowBackendSDLVulkan::SetCursorVisibility(bool visibility) {
    SDL_ShowCursor(visibility ? SDL_ENABLE : SDL_DISABLE);
}

void GfxWindowBackendSDLVulkan::SetMousePos(int32_t posX, int32_t posY) {
    SDL_WarpMouseInWindow(mWnd, posX, posY);
}

void GfxWindowBackendSDLVulkan::GetMousePos(int32_t* x, int32_t* y) {
    SDL_GetMouseState(x, y);
}

void GfxWindowBackendSDLVulkan::GetMouseDelta(int32_t* x, int32_t* y) {
    SDL_GetRelativeMouseState(x, y);
}

void GfxWindowBackendSDLVulkan::GetMouseWheel(float* x, float* y) {
    *x = mMouseWheelX;
    *y = mMouseWheelY;
    mMouseWheelX = 0.0f;
    mMouseWheelY = 0.0f;
}

bool GfxWindowBackendSDLVulkan::GetMouseState(uint32_t btn) {
    return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(btn)) != 0;
}

void GfxWindowBackendSDLVulkan::SetMouseCapture(bool capture) {
    SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
}

bool GfxWindowBackendSDLVulkan::IsMouseCaptured() {
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

void GfxWindowBackendSDLVulkan::GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) {
    SDL_GetWindowSize(mWnd, reinterpret_cast<int*>(width), reinterpret_cast<int*>(height));
    SDL_GetWindowPosition(mWnd, posX, posY);
}

void GfxWindowBackendSDLVulkan::HandleSingleEvent(SDL_Event& event) {
    Ship::GuiWindowInitData dummy;
    Ship::WindowEvent wEvent;
    wEvent.Sdl = { &event };
    Ship::Context::GetInstance()->GetWindow()->GetGui()->HandleWindowEvents(wEvent);

    switch (event.type) {
        case SDL_QUIT:
            mIsRunning = false;
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                mWindowWidth = event.window.data1;
                mWindowHeight = event.window.data2;
            } else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                mIsRunning = false;
            }
            break;
        case SDL_KEYDOWN: {
            int sdlSc = event.key.keysym.scancode;
            int lusSc = (sdlSc < 512) ? mSdlToLusTable[sdlSc] : 0;
            if (mOnKeyDown) {
                mOnKeyDown(lusSc);
            }
            break;
        }
        case SDL_KEYUP: {
            int sdlSc = event.key.keysym.scancode;
            int lusSc = (sdlSc < 512) ? mSdlToLusTable[sdlSc] : 0;
            if (mOnKeyUp) {
                mOnKeyUp(lusSc);
            }
            break;
        }
        case SDL_MOUSEWHEEL:
            mMouseWheelX = event.wheel.preciseX;
            mMouseWheelY = event.wheel.preciseY;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (mOnMouseButtonDown) {
                mOnMouseButtonDown(event.button.button);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (mOnMouseButtonUp) {
                mOnMouseButtonUp(event.button.button);
            }
            break;
        default:
            break;
    }
}

void GfxWindowBackendSDLVulkan::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        HandleSingleEvent(event);
    }
}

bool GfxWindowBackendSDLVulkan::IsFrameReady() {
    return true;
}

void GfxWindowBackendSDLVulkan::SwapBuffersBegin() {
    // Presentation is driven by GfxRenderingAPIVulkan::FinishRender().
}

void GfxWindowBackendSDLVulkan::SwapBuffersEnd() {
}

double GfxWindowBackendSDLVulkan::GetTime() {
    return 0.0;
}

int GfxWindowBackendSDLVulkan::GetTargetFps() {
    return mTargetFps;
}

void GfxWindowBackendSDLVulkan::SetTargetFps(int fps) {
    mTargetFps = fps;
}

void GfxWindowBackendSDLVulkan::SetMaxFrameLatency(int /*latency*/) {
    // Not supported via SDL2.
}

const char* GfxWindowBackendSDLVulkan::GetKeyName(int scancode) {
    // Reverse-lookup LUS→SDL and return SDL name.
    for (size_t i = 0; i < std::size(lus_to_sdl_table_vk); i++) {
        if (mSdlToLusTable[lus_to_sdl_table_vk[i]] == scancode) {
            return SDL_GetScancodeName(lus_to_sdl_table_vk[i]);
        }
    }
    return "";
}

bool GfxWindowBackendSDLVulkan::CanDisableVsync() {
    return true;
}

bool GfxWindowBackendSDLVulkan::IsRunning() {
    return mIsRunning;
}

void GfxWindowBackendSDLVulkan::Destroy() {
    SDL_DestroyWindow(mWnd);
    mWnd = nullptr;
    SDL_Quit();
}

bool GfxWindowBackendSDLVulkan::IsFullscreen() {
    return mFullScreen;
}

} // namespace Fast
#endif // ENABLE_VULKAN
