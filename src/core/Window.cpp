#include <string>
#include <chrono>
#include <fstream>
#include <iostream>
#include "core/Window.h"
#include "controller/KeyboardController.h"
#include "graphic/Fast3D/gfx_pc.h"
#include "graphic/Fast3D/gfx_sdl.h"
#include "graphic/Fast3D/gfx_dxgi.h"
#include "graphic/Fast3D/gfx_glx.h"
#include "graphic/Fast3D/gfx_opengl.h"
#include "graphic/Fast3D/gfx_metal.h"
#include "graphic/Fast3D/gfx_direct3d11.h"
#include "graphic/Fast3D/gfx_direct3d12.h"
#include "graphic/Fast3D/gfx_wiiu.h"
#include "graphic/Fast3D/gfx_gx2.h"
#include "graphic/Fast3D/gfx_rendering_api.h"
#include "graphic/Fast3D/gfx_window_manager_api.h"
#include "gui/Gui.h"
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

    InitWindowManager(GetContext()->GetConfig()->getString("Window.GfxBackend"),
                      GetContext()->GetConfig()->getString("Window.GfxApi"));

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

void Window::InitWindowManager(std::string windowManagerBackend, std::string gfxApiBackend) {
    // Param can override
    mWindowManagerName = windowManagerBackend;
    mRenderingApiName = gfxApiBackend;
#ifdef ENABLE_DX11
    if (mWindowManagerName == "dx11") {
        mRenderingApi = &gfx_direct3d11_api;
        mWindowManagerApi = &gfx_dxgi_api;
        return;
    }
#endif
#if defined(ENABLE_OPENGL) || defined(__APPLE__)
    if (mWindowManagerName == "sdl") {
        mRenderingApi = &gfx_opengl_api;
        mWindowManagerApi = &gfx_sdl;
#ifdef __APPLE__
        if (mRenderingApiName == "Metal" && Metal_IsSupported()) {
            mRenderingApi = &gfx_metal_api;
        }
#endif
        return;
    }
#if defined(__linux__) && defined(X11_SUPPORTED)
    if (mWindowManagerName == "glx") {
        mRenderingApi = &gfx_opengl_api;
        mWindowManagerApi = &gfx_glx;
        return;
    }
#endif
#endif

    // Defaults if not on list above
#if defined(ENABLE_OPENGL) || defined(__APPLE__)
    mRenderingApi = &gfx_opengl_api;
#ifdef __APPLE__
    if (Metal_IsSupported()) {
        mRenderingApi = &gfx_metal_api;
    }
#endif
#if defined(__linux__) && defined(X11_SUPPORTED)
    // LINUX_TODO:
    // *mWindowManagerApi = &gfx_glx;
    mWindowManagerApi = &gfx_sdl;
#else
    mWindowManagerApi = &gfx_sdl;
#endif
#endif
#ifdef ENABLE_DX12
    mRenderingApi = &gfx_direct3d12_api;
    mWindowManagerApi = &gfx_dxgi_api;
#endif
#ifdef ENABLE_DX11
    mRenderingApi = &gfx_direct3d11_api;
    mWindowManagerApi = &gfx_dxgi_api;
#endif
#ifdef __WIIU__
    mRenderingApi = &gfx_gx2_api;
    mWindowManagerApi = &gfx_wiiu;
#endif
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

std::string Window::GetWindowManagerName() {
    return mWindowManagerName;
}

std::string Window::GetRenderingApiName() {
    return mRenderingApiName;
}

bool Window::SupportsWindowedFullscreen() {
#ifdef __SWITCH__
    return false;
#endif

    if (mWindowManagerName == "sdl") {
        return true;
    }

    return false;
}
} // namespace LUS
