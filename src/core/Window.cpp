#include "core/Window.h"
#include <string>
#include <fstream>
#include <iostream>
#include "controller/KeyboardController.h"
#include "graphic/Fast3D/gfx_pc.h"
#include "graphic/Fast3D/gfx_sdl.h"
#include "graphic/Fast3D/gfx_dxgi.h"
#include "graphic/Fast3D/gfx_opengl.h"
#include "graphic/Fast3D/gfx_metal.h"
#include "graphic/Fast3D/gfx_direct3d11.h"
#include "graphic/Fast3D/gfx_direct3d12.h"

#include "core/Context.h"

#ifdef __APPLE__
#include "misc/OSXFolderManager.h"
#elif defined(__SWITCH__)
#include "port/switch/SwitchImpl.h"
#elif defined(__WIIU__)
#include "port/wiiu/WiiUImpl.h"
#endif

namespace LUS {

Window::Window(std::shared_ptr<Context> context) {
    mContext = context;
    mWindowManagerApi = nullptr;
    mRenderingApi = nullptr;
    mIsFullscreen = false;
    mWidth = 320;
    mHeight = 240;
}

Window::~Window() {
    SPDLOG_DEBUG("destruct window");
    spdlog::shutdown();
}

void Window::Init() {
    bool steamDeckGameMode = false;

#ifdef __linux__
    std::ifstream osReleaseFile("/etc/os-release");
    if (osReleaseFile.is_open()) {
        std::string line;
        while (std::getline(osReleaseFile, line)) {
            if (line.find("VARIANT_ID") != std::string::npos) {
                if (line.find("steamdeck") != std::string::npos) {
                    steamDeckGameMode = std::getenv("XDG_CURRENT_DESKTOP") != nullptr &&
                                        std::string(std::getenv("XDG_CURRENT_DESKTOP")) == "gamescope";
                }
                break;
            }
        }
    }
#endif

    mIsFullscreen = GetContext()->GetConfig()->getBool("Window.Fullscreen.Enabled", false) || steamDeckGameMode;

    if (mIsFullscreen) {
        mWidth = GetContext()->GetConfig()->getInt("Window.Fullscreen.Width", steamDeckGameMode ? 1280 : 1920);
        mHeight = GetContext()->GetConfig()->getInt("Window.Fullscreen.Height", steamDeckGameMode ? 800 : 1080);
    } else {
        mWidth = GetContext()->GetConfig()->getInt("Window.Width", 640);
        mHeight = GetContext()->GetConfig()->getInt("Window.Height", 480);
    }

    WindowBackend backend = DetermineBackendFromConfig();
    InitWindowManager(backend);

    gfx_init(mWindowManagerApi, mRenderingApi, GetContext()->GetName().c_str(), mIsFullscreen, mWidth, mHeight);
    mWindowManagerApi->set_fullscreen_changed_callback(OnFullscreenChanged);
    mWindowManagerApi->set_keyboard_callbacks(KeyDown, KeyUp, AllKeysUp);
}

void Window::Close() {
    mWindowManagerApi->close();
}

void Window::StartFrame() {
    gfx_start_frame();
}

void Window::SetTargetFps(int32_t fps) {
    gfx_set_target_fps(fps);
}

void Window::SetMaximumFrameLatency(int32_t latency) {
    gfx_set_maximum_frame_latency(latency);
}

void Window::GetPixelDepthPrepare(float x, float y) {
    gfx_get_pixel_depth_prepare(x, y);
}

uint16_t Window::GetPixelDepth(float x, float y) {
    return gfx_get_pixel_depth(x, y);
}

void Window::ToggleFullscreen() {
    SetFullscreen(!mIsFullscreen);
}

void Window::SetFullscreen(bool isFullscreen) {
    this->mIsFullscreen = isFullscreen;
    mWindowManagerApi->set_fullscreen(isFullscreen);
}

void Window::SetCursorVisibility(bool visible) {
    mWindowManagerApi->set_cursor_visibility(visible);
}

void Window::MainLoop(void (*mainFunction)(void)) {
    mWindowManagerApi->main_loop(mainFunction);
}

bool Window::KeyUp(int32_t scancode) {
    if (scancode == Context::GetInstance()->GetConfig()->getInt("Shortcuts.Fullscreen", 0x044)) {
        Context::GetInstance()->GetWindow()->ToggleFullscreen();
    }

    Context::GetInstance()->GetWindow()->SetLastScancode(-1);

    bool isProcessed = false;
    auto controlDeck = Context::GetInstance()->GetControlDeck();
    const auto pad = dynamic_cast<KeyboardController*>(
        controlDeck->GetDeviceFromDeviceIndex(controlDeck->GetNumDevices() - 2).get());
    if (pad != nullptr) {
        if (pad->ReleaseButton(scancode)) {
            isProcessed = true;
        }
    }

    return isProcessed;
}

bool Window::KeyDown(int32_t scancode) {
    bool isProcessed = false;
    auto controlDeck = Context::GetInstance()->GetControlDeck();
    const auto pad = dynamic_cast<KeyboardController*>(
        controlDeck->GetDeviceFromDeviceIndex(controlDeck->GetNumDevices() - 2).get());
    if (pad != nullptr) {
        if (pad->PressButton(scancode)) {
            isProcessed = true;
        }
    }

    Context::GetInstance()->GetWindow()->SetLastScancode(scancode);

    return isProcessed;
}

void Window::AllKeysUp(void) {
    auto controlDeck = Context::GetInstance()->GetControlDeck();
    const auto pad = dynamic_cast<KeyboardController*>(
        controlDeck->GetDeviceFromDeviceIndex(controlDeck->GetNumDevices() - 2).get());
    if (pad != nullptr) {
        pad->ReleaseAllButtons();
    }
}

void Window::OnFullscreenChanged(bool isNowFullscreen) {
    std::shared_ptr<Mercury> pConf = Context::GetInstance()->GetConfig();

    Context::GetInstance()->GetWindow()->mIsFullscreen = isNowFullscreen;
    pConf->setBool("Window.Fullscreen.Enabled", isNowFullscreen);
    if (isNowFullscreen) {
        auto menuBar = Context::GetInstance()->GetWindow()->GetGui()->GetMenuBar();
        Context::GetInstance()->GetWindow()->SetCursorVisibility(menuBar && menuBar->IsVisible());
    } else {
        Context::GetInstance()->GetWindow()->SetCursorVisibility(true);
    }
}

uint32_t Window::GetCurrentWidth() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight);
    return mWidth;
}

uint32_t Window::GetCurrentHeight() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight);
    return mHeight;
}

uint32_t Window::GetCurrentRefreshRate() {
    mWindowManagerApi->get_active_window_refresh_rate(&mRefreshRate);
    return mRefreshRate;
}

bool Window::CanDisableVerticalSync() {
    return mWindowManagerApi->can_disable_vsync();
}

float Window::GetCurrentAspectRatio() {
    return (float)GetCurrentWidth() / (float)GetCurrentHeight();
}

WindowBackend Window::DetermineBackendFromConfig() {
    std::string windowManagerName = Context::GetInstance()->GetConfig()->getString("Window.GfxBackend");
    std::string gfxApiName = Context::GetInstance()->GetConfig()->getString("Window.GfxApi");
    if (windowManagerName == "dx11") {
        return WindowBackend::DX11;
    } else if (windowManagerName == "dx12") {
        return WindowBackend::DX12;
    } else if (windowManagerName == "sdl") {
#ifdef __APPLE__
        if (gfxApiName == "Metal" && Metal_IsSupported()) {
            return WindowBackend::SDL_METAL;
        }
#endif
        return WindowBackend::SDL_OPENGL;
    } else if (windowManagerName == "glx") {
        return WindowBackend::GLX_OPENGL;
    } else {
        // Defaults if not on list above
#ifdef ENABLE_DX12
        return WindowBackend::DX12;
#endif
#ifdef ENABLE_DX11
        return WindowBackend::DX11;
#endif
#ifdef __WIIU__
        return WindowBackend::GX2;
#endif
#if defined(ENABLE_OPENGL) || defined(__APPLE__)
#ifdef __APPLE__
        if (Metal_IsSupported()) {
            return WindowBackend::SDL_METAL;
        } else {
            return WindowBackend::SDL_OPENGL;
        }
#endif
#if defined(__linux__) && defined(X11_SUPPORTED)
        // return Window::Backend::GLX_OPENGL;
        return WindowBackend::SDL_OPENGL;
#else
        return WindowBackend::SDL_OPENGL;
#endif
#endif
        SPDLOG_ERROR("Could not determine rendering backend from window manager {} and gfx api {}", windowManagerName,
                     gfxApiName);
    }
}

std::string Window::DetermineWindowManagerFromBackend(WindowBackend backend) {
    switch (backend) {
        case WindowBackend::DX11:
            return "dx11";
        case WindowBackend::DX12:
            return "dx12";
        case WindowBackend::GLX_OPENGL:
            return "glx";
        case WindowBackend::SDL_OPENGL:
            return "sdl";
        case WindowBackend::SDL_METAL:
            return "sdl";
        case WindowBackend::GX2:
            return "gx2";
        default:
            return "";
    }
}

std::string Window::DetermineGraphicsApiFromBackend(WindowBackend backend) {
    switch (backend) {
        case WindowBackend::DX11:
            return "DirectX 11";
        case WindowBackend::DX12:
            return "DirectX 12";
        case WindowBackend::GLX_OPENGL:
            return "OpenGL";
        case WindowBackend::SDL_OPENGL:
            return "OpenGL";
        case WindowBackend::SDL_METAL:
            return "Metal";
        case WindowBackend::GX2:
            return "GX2";
        default:
            return "";
    }
}

void Window::InitWindowManager(WindowBackend backend) {
    SetWindowBackend(backend);
    switch (GetWindowBackend()) {
#ifdef ENABLE_DX11
        case WindowBackend::DX11:
            mRenderingApi = &gfx_direct3d11_api;
            mWindowManagerApi = &gfx_dxgi_api;
            break;
#endif
#ifdef ENABLE_DX12
        case WindowBackend::DX12:
            mRenderingApi = &gfx_direct3d12_api;
            mWindowManagerApi = &gfx_dxgi_api;
            break;
#endif
#if defined(ENABLE_OPENGL) || defined(__APPLE__)
#if defined(__linux__) && defined(X11_SUPPORTED)
        case WindowBackend::GLX_OPENGL:
            mRenderingApi = &gfx_opengl_api;
            mWindowManagerApi = &gfx_glx;
            break;
#endif
        case WindowBackend::SDL_OPENGL:
            mRenderingApi = &gfx_opengl_api;
            mWindowManagerApi = &gfx_sdl;
            break;
#ifdef __APPLE__
        case WindowBackend::SDL_METAL:
            mRenderingApi = &gfx_metal_api;
            mWindowManagerApi = &gfx_sdl;
            break;
#endif
#endif
#ifdef __WIIU__
        case WindowBackend::GX2:
            mRenderingApi = &gfx_gx2_api;
            mWindowManagerApi = &gfx_wiiu;
            break;
#endif
        default:
            SPDLOG_ERROR("Could not load the correct rendering backend");
            break;
    }
}

bool Window::IsFullscreen() {
    return mIsFullscreen;
}

const char* Window::GetKeyName(int32_t scancode) {
    return mWindowManagerApi->get_key_name(scancode);
}

int32_t Window::GetLastScancode() {
    return mLastScancode;
}

void Window::SetLastScancode(int32_t scanCode) {
    mLastScancode = scanCode;
}

std::shared_ptr<Context> Window::GetContext() {
    return mContext;
}

std::shared_ptr<Gui> Window::GetGui() {
    return mGui;
}

bool Window::SupportsWindowedFullscreen() {
#ifdef __SWITCH__
    return false;
#endif

    if (GetWindowBackend() == WindowBackend::SDL_OPENGL || GetWindowBackend() == WindowBackend::SDL_METAL) {
        return true;
    }

    return false;
}

void Window::SetResolutionMultiplier(float multiplier) {
    gfx_current_dimensions.internal_mul = multiplier;
}

void Window::SetMsaaLevel(uint32_t value) {
    gfx_msaa_level = value;
}

WindowBackend Window::GetWindowBackend() {
    return mWindowBackend;
}

void Window::SetWindowBackend(WindowBackend backend) {
    std::string gfxBackend = DetermineWindowManagerFromBackend(backend);
    std::string gfxApi = DetermineGraphicsApiFromBackend(backend);
    Context::GetInstance()->GetConfig()->setString("Window.GfxBackend", gfxBackend);
    Context::GetInstance()->GetConfig()->setString("Window.GfxApi", gfxApi);
    mWindowBackend = backend;
}
} // namespace LUS
