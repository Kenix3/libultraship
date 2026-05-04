#include "fast/Fast3dWindow.h"

#include "ship/Context.h"
#include "ship/config/Config.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/config/ConsoleVariable.h"
#include "fast/interpreter.h"
#include "fast/backends/gfx_sdl.h"
#include "fast/backends/gfx_dxgi.h"
#include "fast/backends/gfx_opengl.h"
#include "fast/backends/gfx_metal.h"
#include "fast/backends/gfx_direct3d_common.h"
#include "fast/backends/gfx_direct3d11.h"
#include "fast/backends/gfx_window_manager_api.h"

#ifdef __APPLE__
#include <SDL_hints.h>
#include <SDL_video.h>
#include <imgui_impl_metal.h>
#include <imgui_impl_sdl2.h>
#else
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_video.h>
#endif

#if defined(__ANDROID__) || defined(__IOS__)
#include "ship/port/mobile/MobileImpl.h"
#endif

#ifdef ENABLE_OPENGL
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#endif

#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

// NOLINTNEXTLINE
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#include <fstream>

namespace Fast {

extern void GfxSetInstance(std::shared_ptr<Interpreter> gfx);

Fast3dWindow::Fast3dWindow(std::shared_ptr<Ship::Gui> gui, std::shared_ptr<FastMouseStateManager> mouseStateManager)
    : Ship::Window(gui, mouseStateManager) {
    mWindowManagerApi = nullptr;
    mRenderingApi = nullptr;
    mInterpreter = std::make_shared<Interpreter>();
    GfxSetInstance(mInterpreter);

#ifdef _WIN32
    AddAvailableWindowBackend(WindowBackend::FAST3D_DXGI_DX11);
#endif
#ifdef __APPLE__
    if (Metal_IsSupported()) {
        AddAvailableWindowBackend(WindowBackend::FAST3D_SDL_METAL);
    }
#endif
    AddAvailableWindowBackend(WindowBackend::FAST3D_SDL_OPENGL);
}

Fast3dWindow::Fast3dWindow(std::shared_ptr<Ship::Gui> gui)
    : Fast3dWindow(gui, std::make_shared<FastMouseStateManager>()) {
}

Fast3dWindow::Fast3dWindow(std::vector<std::shared_ptr<Ship::GuiWindow>> guiWindows)
    : Fast3dWindow(std::make_shared<Ship::Gui>(guiWindows)) {
}

Fast3dWindow::Fast3dWindow() : Fast3dWindow(std::vector<std::shared_ptr<Ship::GuiWindow>>()) {
}

Fast3dWindow::~Fast3dWindow() {
    SPDLOG_DEBUG("destruct fast3dwindow");
    mInterpreter->Destroy();
    delete mRenderingApi;
    delete mWindowManagerApi;
}

void Fast3dWindow::Init() {
    bool gameMode = false;

#ifdef __linux__
    std::ifstream osReleaseFile("/etc/os-release");
    if (osReleaseFile.is_open()) {
        std::string line;
        while (std::getline(osReleaseFile, line)) {
            if (line.find("VARIANT_ID") != std::string::npos) {
                if (line.find("steamdeck") != std::string::npos) {
                    gameMode = std::getenv("XDG_CURRENT_DESKTOP") != nullptr &&
                               std::string(std::getenv("XDG_CURRENT_DESKTOP")) == "gamescope";
                }
                break;
            }
        }
    }
#elif defined(__ANDROID__) || defined(__IOS__)
    gameMode = true;
#endif

    bool isFullscreen;
    uint32_t width, height;
    int32_t posX, posY;

    isFullscreen = Ship::Context::GetInstance()->GetConfig()->GetBool("Window.Fullscreen.Enabled", false) || gameMode;
    posX = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.PositionX", 100);
    posY = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.PositionY", 100);

    if (isFullscreen) {
        width = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Fullscreen.Width", gameMode ? 1280 : 1920);
        height = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Fullscreen.Height", gameMode ? 800 : 1080);
    } else {
        width = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Width", 640);
        height = Ship::Context::GetInstance()->GetConfig()->GetInt("Window.Height", 480);
    }
    Ship::Context::GetInstance()->GetWindow()->SetFullscreenScancode(
        Ship::Context::GetInstance()->GetConfig()->GetInt("Shortcuts.Fullscreen", Ship::KbScancode::LUS_KB_F11));
    Ship::Context::GetInstance()->GetWindow()->SetMouseCaptureScancode(
        Ship::Context::GetInstance()->GetConfig()->GetInt("Shortcuts.MouseCapture", Ship::KbScancode::LUS_KB_F2));

    InitWindowManager();
    mInterpreter->Init(mWindowManagerApi, mRenderingApi, Ship::Context::GetInstance()->GetName().c_str(), isFullscreen,
                       width, height, posX, posY);
    mWindowManagerApi->SetFullscreenChangedCallback(OnFullscreenChanged);
    mWindowManagerApi->SetKeyboardCallbacks(KeyDown, KeyUp, AllKeysUp);
    mWindowManagerApi->SetMouseCallbacks(MouseButtonDown, MouseButtonUp);

    SetTextureFilter((FilteringMode)Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
        CVAR_TEXTURE_FILTER, FILTER_THREE_POINT));
}

int32_t Fast3dWindow::GetTargetFps() {
    return mInterpreter->GetTargetFps();
}

void Fast3dWindow::SetTargetFps(int32_t fps) {
    mInterpreter->SetTargetFps(fps);
}

void Fast3dWindow::SetMaximumFrameLatency(int32_t latency) {
    mInterpreter->SetMaxFrameLatency(latency);
}

void Fast3dWindow::GetPixelDepthPrepare(float x, float y) {
    mInterpreter->GetPixelDepthPrepare(x, y);
}

uint16_t Fast3dWindow::GetPixelDepth(float x, float y) {
    return mInterpreter->GetPixelDepth(x, y);
}

void Fast3dWindow::InitWindowManager() {
    SetWindowBackend(Ship::Context::GetInstance()->GetConfig()->GetWindowBackend());

    switch (GetWindowBackend()) {
#ifdef ENABLE_DX11
        case WindowBackend::FAST3D_DXGI_DX11:
            mWindowManagerApi = new GfxWindowBackendDXGI();
            mRenderingApi = new GfxRenderingAPIDX11(static_cast<GfxWindowBackendDXGI*>(mWindowManagerApi));
            break;
#endif
#ifdef ENABLE_OPENGL
        case WindowBackend::FAST3D_SDL_OPENGL:
            mRenderingApi = new GfxRenderingAPIOGL();
            mWindowManagerApi = new GfxWindowBackendSDL2();
            break;
#endif
#ifdef __APPLE__
        case WindowBackend::FAST3D_SDL_METAL:
            mRenderingApi = new GfxRenderingAPIMetal();
            mWindowManagerApi = new GfxWindowBackendSDL2();
            break;
#endif
        default:
            SPDLOG_ERROR("Could not load the correct rendering backend");
            break;
    }
}

void Fast3dWindow::SetTextureFilter(FilteringMode filteringMode) {
    mInterpreter->GetCurrentRenderingAPI()->SetTextureFilter(filteringMode);
}

void Fast3dWindow::EnableSRGBMode() {
    mInterpreter->mRapi->SetSrgbMode();
}

void Fast3dWindow::SetRendererUCode(UcodeHandlers ucode) {
    gfx_set_target_ucode(ucode);
}

void Fast3dWindow::Close() {
    mWindowManagerApi->Close();
}

void Fast3dWindow::RunGuiOnly() {
    mInterpreter->RunGuiOnly();
}

void Fast3dWindow::StartFrame() {
    mInterpreter->StartFrame();
}

void Fast3dWindow::EndFrame() {
    mInterpreter->EndFrame();
}

bool Fast3dWindow::IsFrameReady() {
    return mWindowManagerApi->IsFrameReady();
}

bool Fast3dWindow::DrawAndRunGraphicsCommands(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtxReplacements) {
    std::shared_ptr<Window> wnd = Ship::Context::GetInstance()->GetWindow();

    // Skip dropped frames
    if (!wnd->IsFrameReady()) {
        return false;
    }

    auto gui = wnd->GetGui();
    // Setup mouse state manager
    wnd->GetMouseStateManager()->StartFrame();
    // Setup of the backend frames and draw initial Window and GUI menus
    gui->StartDraw();
    // Setup game framebuffers to match available window space
    mInterpreter->StartFrame();
    // Execute the games gfx commands
    mInterpreter->Run(commands, mtxReplacements);
    // Renders the game frame buffer to the final window and finishes the GUI
    gui->EndDraw();
    // Finalize swap buffers
    mInterpreter->EndFrame();

    return true;
}

void Fast3dWindow::HandleEvents() {
    mWindowManagerApi->HandleEvents();
}

void Fast3dWindow::SetCursorVisibility(bool visible) {
    mWindowManagerApi->SetCursorVisibility(visible);
}

uint32_t Fast3dWindow::GetWidth() {
    uint32_t width, height;
    int32_t posX, posY;
    mWindowManagerApi->GetDimensions(&width, &height, &posX, &posY);
    return width;
}

uint32_t Fast3dWindow::GetHeight() {
    uint32_t width, height;
    int32_t posX, posY;
    mWindowManagerApi->GetDimensions(&width, &height, &posX, &posY);
    return height;
}

float Fast3dWindow::GetAspectRatio() {
    return mInterpreter->mCurDimensions.aspect_ratio;
}

int32_t Fast3dWindow::GetPosX() {
    uint32_t width, height;
    int32_t posX, posY;
    mWindowManagerApi->GetDimensions(&width, &height, &posX, &posY);
    return posX;
}

int32_t Fast3dWindow::GetPosY() {
    uint32_t width, height;
    int32_t posX, posY;
    mWindowManagerApi->GetDimensions(&width, &height, &posX, &posY);
    return posY;
}

void Fast3dWindow::SetMousePos(Ship::Coords pos) {
    mWindowManagerApi->SetMousePos(pos.x, pos.y);
}

Ship::Coords Fast3dWindow::GetMousePos() {
    int32_t x, y;
    mWindowManagerApi->GetMousePos(&x, &y);
    return { x, y };
}

Ship::Coords Fast3dWindow::GetMouseDelta() {
    int32_t x, y;
    mWindowManagerApi->GetMouseDelta(&x, &y);
    return { x, y };
}

Ship::CoordsF Fast3dWindow::GetMouseWheel() {
    float x, y;
    mWindowManagerApi->GetMouseWheel(&x, &y);
    return { x, y };
}

bool Fast3dWindow::GetMouseState(Ship::MouseBtn btn) {
    return mWindowManagerApi->GetMouseState(static_cast<uint32_t>(btn));
}

void Fast3dWindow::SetMouseCapture(bool capture) {
    mWindowManagerApi->SetMouseCapture(capture);
}

bool Fast3dWindow::IsMouseCaptured() {
    return mWindowManagerApi->IsMouseCaptured();
}

uint32_t Fast3dWindow::GetCurrentRefreshRate() {
    uint32_t refreshRate;
    mWindowManagerApi->GetActiveWindowRefreshRate(&refreshRate);
    return refreshRate;
}

bool Fast3dWindow::SupportsWindowedFullscreen() {
#ifdef __APPLE__
    return false;
#endif

    if (GetWindowBackend() == WindowBackend::FAST3D_SDL_OPENGL) {
        return true;
    }

    return false;
}

bool Fast3dWindow::CanDisableVerticalSync() {
    return mWindowManagerApi->CanDisableVsync();
}

void Fast3dWindow::SetResolutionMultiplier(float multiplier) {
    mInterpreter->SetResolutionMultiplier(multiplier);
}

void Fast3dWindow::SetMsaaLevel(uint32_t value) {
    mInterpreter->SetMsaaLevel(value);
}

void Fast3dWindow::SetFullscreen(bool isFullscreen) {
    // Save current window position before fullscreening
    SaveWindowToConfig();
    mWindowManagerApi->SetFullscreen(isFullscreen);
}

bool Fast3dWindow::IsFullscreen() {
    return mWindowManagerApi->IsFullscreen();
}

bool Fast3dWindow::IsRunning() {
    return mWindowManagerApi->IsRunning();
}

uintptr_t Fast3dWindow::GetGfxFrameBuffer() {
    return mInterpreter->mGfxFrameBuffer;
}

const char* Fast3dWindow::GetKeyName(int32_t scancode) {
    return mWindowManagerApi->GetKeyName(scancode);
}

bool Fast3dWindow::KeyUp(int32_t scancode) {
    if (scancode == Ship::Context::GetInstance()->GetWindow()->GetFullscreenScancode()) {
        Ship::Context::GetInstance()->GetWindow()->ToggleFullscreen();
    }

    if (scancode == Ship::Context::GetInstance()->GetWindow()->GetMouseCaptureScancode()) {
        Ship::Context::GetInstance()->GetWindow()->GetMouseStateManager()->ToggleMouseCaptureOverride();
    }

    Ship::Context::GetInstance()->GetWindow()->SetLastScancode(-1);
    return Ship::Context::GetInstance()->GetControlDeck()->ProcessKeyboardEvent(
        Ship::KbEventType::LUS_KB_EVENT_KEY_UP, static_cast<Ship::KbScancode>(scancode));
}

bool Fast3dWindow::KeyDown(int32_t scancode) {
    bool isProcessed = Ship::Context::GetInstance()->GetControlDeck()->ProcessKeyboardEvent(
        Ship::KbEventType::LUS_KB_EVENT_KEY_DOWN, static_cast<Ship::KbScancode>(scancode));
    Ship::Context::GetInstance()->GetWindow()->SetLastScancode(scancode);

    return isProcessed;
}

void Fast3dWindow::AllKeysUp() {
    Ship::Context::GetInstance()->GetControlDeck()->ProcessKeyboardEvent(Ship::KbEventType::LUS_KB_EVENT_ALL_KEYS_UP,
                                                                         Ship::KbScancode::LUS_KB_UNKNOWN);
}

bool Fast3dWindow::MouseButtonUp(int button) {
    return Ship::Context::GetInstance()->GetControlDeck()->ProcessMouseButtonEvent(false,
                                                                                   static_cast<Ship::MouseBtn>(button));
}

bool Fast3dWindow::MouseButtonDown(int button) {
    bool isProcessed = Ship::Context::GetInstance()->GetControlDeck()->ProcessMouseButtonEvent(
        true, static_cast<Ship::MouseBtn>(button));
    return isProcessed;
}

void Fast3dWindow::OnFullscreenChanged(bool isNowFullscreen) {
    std::shared_ptr<Window> wnd = Ship::Context::GetInstance()->GetWindow();

    // Re-save fullscreen enabled after
    Ship::Context::GetInstance()->GetConfig()->SetBool("Window.Fullscreen.Enabled", isNowFullscreen);
}

std::weak_ptr<Interpreter> Fast3dWindow::GetInterpreterWeak() const {
    return mInterpreter;
}

bool Fast3dWindow::SupportsViewports() {
    return true;
}

void Fast3dWindow::HandleWindowEvents(Ship::WindowEvent event) {
    switch (GetWindowBackend()) {
        case WindowBackend::FAST3D_SDL_OPENGL:
        case WindowBackend::FAST3D_SDL_METAL:
            ImGui_ImplSDL2_ProcessEvent(static_cast<const SDL_Event*>(event.Sdl.Event));
#if defined(__ANDROID__) || defined(__IOS__)
            Mobile::ImGuiProcessEvent(ImGui::GetIO().WantTextInput);
#endif
            break;
#ifdef ENABLE_DX11
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(event.Win32.Handle), event.Win32.Msg, event.Win32.Param1,
                                           event.Win32.Param2);
            break;
#endif
        default:
            break;
    }
}

void Fast3dWindow::ImGuiWMInit(Ship::GuiWindowInitData windowImpl) {
    switch (GetWindowBackend()) {
        case WindowBackend::FAST3D_SDL_OPENGL:
            SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
            if (Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_ALLOW_BACKGROUND_INPUTS, 1)) {
                SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
            }
            ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(windowImpl.Opengl.Window),
                                         windowImpl.Opengl.Context);
            break;
#if __APPLE__
        case WindowBackend::FAST3D_SDL_METAL:
            SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
            if (Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_ALLOW_BACKGROUND_INPUTS, 1)) {
                SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
            }
            ImGui_ImplSDL2_InitForMetal(static_cast<SDL_Window*>(windowImpl.Metal.Window));
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplWin32_Init(windowImpl.Dx11.Window);
            break;
#endif
        default:
            break;
    }
}

void Fast3dWindow::ImGuiWMShutdown() {
    switch (GetWindowBackend()) {
#ifdef ENABLE_OPENGL
        case WindowBackend::FAST3D_SDL_OPENGL:
            ImGui_ImplSDL2_Shutdown();
            break;
#endif
#if __APPLE__
        case WindowBackend::FAST3D_SDL_METAL:
            ImGui_ImplSDL2_Shutdown();
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplWin32_Shutdown();
            break;
#endif
        default:
            break;
    }
}

void Fast3dWindow::ImGuiBackendInit(Ship::GuiWindowInitData windowImpl) {
    switch (GetWindowBackend()) {
#ifdef ENABLE_OPENGL
        case WindowBackend::FAST3D_SDL_OPENGL:
#ifdef __APPLE__
            ImGui_ImplOpenGL3_Init("#version 410 core");
#elif USE_OPENGLES
            ImGui_ImplOpenGL3_Init("#version 300 es");
#else
            ImGui_ImplOpenGL3_Init("#version 120");
#endif
            break;
#endif

#ifdef __APPLE__
        case WindowBackend::FAST3D_SDL_METAL: {
            GfxRenderingAPIMetal* api = (GfxRenderingAPIMetal*)mInterpreter->GetCurrentRenderingAPI();
            api->MetalInit(windowImpl.Metal.Renderer);
            break;
        }
#endif

#ifdef ENABLE_DX11
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(windowImpl.Dx11.Device),
                                static_cast<ID3D11DeviceContext*>(windowImpl.Dx11.DeviceContext));
            break;
#endif
        default:
            break;
    }
}

void Fast3dWindow::ImGuiBackendShutdown() {
    switch (GetWindowBackend()) {
#ifdef ENABLE_OPENGL
        case WindowBackend::FAST3D_SDL_OPENGL:
            ImGui_ImplOpenGL3_Shutdown();
            break;
#endif
#if __APPLE__
        case WindowBackend::FAST3D_SDL_METAL:
            ImGui_ImplMetal_Shutdown();
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplDX11_Shutdown();
            break;
#endif
        default:
            break;
    }
}

void Fast3dWindow::ImGuiBackendNewFrame() {
    switch (GetWindowBackend()) {
#ifdef ENABLE_OPENGL
        case WindowBackend::FAST3D_SDL_OPENGL:
            ImGui_ImplOpenGL3_NewFrame();
            break;
#endif

#ifdef ENABLE_DX11
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplDX11_NewFrame();
            break;
#endif

#ifdef __APPLE__
        case WindowBackend::FAST3D_SDL_METAL: {
            GfxRenderingAPIMetal* api = (GfxRenderingAPIMetal*)mInterpreter->GetCurrentRenderingAPI();
            api->NewFrame();
            break;
        }
#endif
        default:
            break;
    }
}

void Fast3dWindow::ImGuiWMNewFrame() {
    switch (GetWindowBackend()) {
        case WindowBackend::FAST3D_SDL_OPENGL:
        case WindowBackend::FAST3D_SDL_METAL:
            ImGui_ImplSDL2_NewFrame();
            break;
#ifdef ENABLE_DX11
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplWin32_NewFrame();
            break;
#endif
        default:
            break;
    }
}

void Fast3dWindow::ImGuiRenderDrawData(ImDrawData* data) {
    switch (GetWindowBackend()) {
#ifdef ENABLE_OPENGL
        case WindowBackend::FAST3D_SDL_OPENGL:
            ImGui_ImplOpenGL3_RenderDrawData(data);
            break;
#endif

#ifdef __APPLE__
        case WindowBackend::FAST3D_SDL_METAL: {
            GfxRenderingAPIMetal* api = (GfxRenderingAPIMetal*)mInterpreter->GetCurrentRenderingAPI();
            api->RenderDrawData(data);
            break;
        }
#endif

#ifdef ENABLE_DX11
        case WindowBackend::FAST3D_DXGI_DX11:
            ImGui_ImplDX11_RenderDrawData(data);
            break;
#endif
        default:
            break;
    }
}

void Fast3dWindow::DrawFloatingWindows(Ship::GuiWindowInitData windowImpl) {
    // OpenGL requires extra platform handling for the GL context
    if (GetWindowBackend() == WindowBackend::FAST3D_SDL_OPENGL && windowImpl.Opengl.Context != nullptr) {
        // Backup window and context before calling RenderPlatformWindowsDefault
        SDL_Window* backupCurrentWindow = SDL_GL_GetCurrentWindow();
        SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        // Restore GL context for next frame
        SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
    } else {
#ifdef __APPLE__
        // Metal requires additional frame setup to get ImGui ready for drawing floating windows
        if (GetWindowBackend() == WindowBackend::FAST3D_SDL_METAL) {
            GfxRenderingAPIMetal* api = (GfxRenderingAPIMetal*)mInterpreter->GetCurrentRenderingAPI();
            api->SetupFloatingFrame();
        }
#endif

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

} // namespace Fast
