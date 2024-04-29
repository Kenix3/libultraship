#include "window/Window.h"
#include <string>
#include <fstream>
#include <iostream>
#include "public/bridge/consolevariablebridge.h"
#include "graphic/Fast3D/gfx_pc.h"
#include "graphic/Fast3D/gfx_sdl.h"
#include "graphic/Fast3D/gfx_dxgi.h"
#include "graphic/Fast3D/gfx_opengl.h"
#include "graphic/Fast3D/gfx_metal.h"
#include "graphic/Fast3D/gfx_direct3d11.h"
#include "graphic/Fast3D/gfx_direct3d12.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "Context.h"

#ifdef __APPLE__
#include "utils/AppleFolderManager.h"
#endif

namespace Ship {

Window::Window(std::shared_ptr<GuiWindow> customInputEditorWindow) {
    mWindowManagerApi = nullptr;
    mRenderingApi = nullptr;
    mIsFullscreen = false;
    mWidth = 320;
    mHeight = 240;
    mPosX = 100;
    mPosY = 100;
    mGui = std::make_shared<Gui>(customInputEditorWindow);
}

Window::Window() : Window(nullptr) {
}

Window::~Window() {
    gfx_destroy();
    SPDLOG_DEBUG("destruct window");
}

void Window::Init() {
    bool mGameMode = false;

#ifdef __linux__
    std::ifstream osReleaseFile("/etc/os-release");
    if (osReleaseFile.is_open()) {
        std::string line;
        while (std::getline(osReleaseFile, line)) {
            if (line.find("VARIANT_ID") != std::string::npos) {
                if (line.find("steamdeck") != std::string::npos) {
                    mGameMode = std::getenv("XDG_CURRENT_DESKTOP") != nullptr &&
                                std::string(std::getenv("XDG_CURRENT_DESKTOP")) == "gamescope";
                }
                break;
            }
        }
    }
#elif defined(__ANDROID__) || defined(__IOS__)
    mGameMode = true;
#endif

    mIsFullscreen = Ship::Context::GetInstance()->GetConfig()->GetBool("Window.Fullscreen.Enabled", false) || mGameMode;
    mPosX = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.PositionX", mPosX);
    mPosY = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.PositionY", mPosY);

    if (mIsFullscreen) {
        mWidth = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Fullscreen.Width", mGameMode ? 1280 : 1920);
        mHeight = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Fullscreen.Height", mGameMode ? 800 : 1080);
    } else {
        mWidth = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Width", 640);
        mHeight = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Height", 480);
    }

    mAvailableWindowBackends = std::make_shared<std::vector<WindowBackend>>();
#ifdef _WIN32
    mAvailableWindowBackends->push_back(WindowBackend::DX11);
#endif
#ifdef __APPLE__
    if (Metal_IsSupported()) {
        mAvailableWindowBackends->push_back(WindowBackend::SDL_METAL);
    }
#endif
    mAvailableWindowBackends->push_back(WindowBackend::SDL_OPENGL);

    InitWindowManager();

    gfx_init(mWindowManagerApi, mRenderingApi, Ship::Context::GetInstance()->GetName().c_str(), mIsFullscreen, mWidth,
             mHeight, mPosX, mPosY);
    mWindowManagerApi->set_fullscreen_changed_callback(OnFullscreenChanged);
    mWindowManagerApi->set_keyboard_callbacks(KeyDown, KeyUp, AllKeysUp);

    SetTextureFilter((FilteringMode)CVarGetInteger("gTextureFilter", FILTER_THREE_POINT));
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
    SetFullscreen(!IsFullscreen());
}

void Window::SetFullscreen(bool isFullscreen) {
    SaveWindowSizeToConfig(Ship::Context::GetInstance()->GetConfig());
    mWindowManagerApi->set_fullscreen(isFullscreen);
}

void Window::SetCursorVisibility(bool visible) {
    mWindowManagerApi->set_cursor_visibility(visible);
}

void Window::MainLoop(void (*mainFunction)(void)) {
    mWindowManagerApi->main_loop(mainFunction);
}

bool Window::KeyUp(int32_t scancode) {
    if (scancode == Context::GetInstance()->GetConfig()->GetInt("Shortcuts.Fullscreen", KbScancode::LUS_KB_F11)) {
        Context::GetInstance()->GetWindow()->ToggleFullscreen();
    }

    Context::GetInstance()->GetWindow()->SetLastScancode(-1);
    return Context::GetInstance()->GetControlDeck()->ProcessKeyboardEvent(KbEventType::LUS_KB_EVENT_KEY_UP,
                                                                          static_cast<KbScancode>(scancode));
}

bool Window::KeyDown(int32_t scancode) {
    bool isProcessed = Context::GetInstance()->GetControlDeck()->ProcessKeyboardEvent(
        KbEventType::LUS_KB_EVENT_KEY_DOWN, static_cast<KbScancode>(scancode));
    Context::GetInstance()->GetWindow()->SetLastScancode(scancode);

    return isProcessed;
}

void Window::AllKeysUp(void) {
    Context::GetInstance()->GetControlDeck()->ProcessKeyboardEvent(KbEventType::LUS_KB_EVENT_ALL_KEYS_UP,
                                                                   KbScancode::LUS_KB_UNKNOWN);
}

void Window::OnFullscreenChanged(bool isNowFullscreen) {
    std::shared_ptr<Window> wnd = Context::GetInstance()->GetWindow();
    std::shared_ptr<Config> conf = Context::GetInstance()->GetConfig();

    wnd->mIsFullscreen = isNowFullscreen;
    if (isNowFullscreen) {
        auto menuBar = wnd->GetGui()->GetMenuBar();
        wnd->SetCursorVisibility(menuBar && menuBar->IsVisible());
    } else {
        wnd->SetCursorVisibility(true);
    }
}

uint32_t Window::GetWidth() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight, &mPosX, &mPosY);
    return mWidth;
}

uint32_t Window::GetHeight() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight, &mPosX, &mPosY);
    return mHeight;
}

int32_t Window::GetPosX() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight, &mPosX, &mPosY);
    return mPosX;
}

int32_t Window::GetPosY() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight, &mPosX, &mPosY);
    return mPosY;
}

uint32_t Window::GetCurrentRefreshRate() {
    mWindowManagerApi->get_active_window_refresh_rate(&mRefreshRate);
    return mRefreshRate;
}

bool Window::CanDisableVerticalSync() {
    return mWindowManagerApi->can_disable_vsync();
}

float Window::GetCurrentAspectRatio() {
    return (float)GetWidth() / (float)GetHeight();
}

void Window::InitWindowManager() {
    SetWindowBackend(Context::GetInstance()->GetConfig()->GetWindowBackend());

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
#ifdef ENABLE_OPENGL
        case WindowBackend::SDL_OPENGL:
            mRenderingApi = &gfx_opengl_api;
            mWindowManagerApi = &gfx_sdl;
            break;
#endif
#ifdef __APPLE__
        case WindowBackend::SDL_METAL:
            mRenderingApi = &gfx_metal_api;
            mWindowManagerApi = &gfx_sdl;
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

std::shared_ptr<Gui> Window::GetGui() {
    return mGui;
}

bool Window::SupportsWindowedFullscreen() {
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

void Window::SetTextureFilter(FilteringMode filteringMode) {
    gfx_get_current_rendering_api()->set_texture_filter(filteringMode);
}

WindowBackend Window::GetWindowBackend() {
    return mWindowBackend;
}

std::shared_ptr<std::vector<WindowBackend>> Window::GetAvailableWindowBackends() {
    return mAvailableWindowBackends;
}

void Window::SetWindowBackend(WindowBackend backend) {
    mWindowBackend = backend;
    Context::GetInstance()->GetConfig()->SetWindowBackend(GetWindowBackend());
    Context::GetInstance()->GetConfig()->Save();
}

void Window::SaveWindowSizeToConfig(std::shared_ptr<Config> conf) {
    // This accepts conf in because it can be run in the destruction of LUS.
    conf->SetBool("Window.Fullscreen.Enabled", IsFullscreen());
    if (IsFullscreen()) {
        conf->SetInt("Window.Fullscreen.Width", (int32_t)GetWidth());
        conf->SetInt("Window.Fullscreen.Height", (int32_t)GetHeight());
    } else {
        conf->SetInt("Window.Width", (int32_t)GetWidth());
        conf->SetInt("Window.Height", (int32_t)GetHeight());
        conf->SetInt("Window.PositionX", GetPosX());
        conf->SetInt("Window.PositionY", GetPosY());
    }
}
} // namespace Ship
