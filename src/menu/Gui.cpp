#define NOMINMAX

#include "Gui.h"

#include <cstring>
#include <utility>
#include <string>
#include <vector>

#include "Mercury.h"
#include "core/Window.h"
#include "menu/ConsoleWindow.h"
#include "misc/Hooks.h"
#include "core/Context.h"
#include "resource/ResourceManager.h"
#include "core/bridge/consolevariablebridge.h"
#include "resource/type/Texture.h"
#include "graphic/Fast3D/gfx_pc.h"
#include "resource/File.h"
#include <stb/stb_image.h>
#include "graphic/Fast3D/gfx_rendering_api.h"
#include "menu/Fonts.h"

#ifdef __WIIU__
#include <gx2/registers.h> // GX2SetViewport / GX2SetScissor

#include <ImGui/backends/wiiu/imgui_impl_gx2.h>
#include <ImGui/backends/wiiu/imgui_impl_wiiu.h>

#include "graphic/Fast3D/gfx_wiiu.h"
#include "graphic/Fast3D/gfx_gx2.h"
#endif

#ifdef __APPLE__
#include <SDL_hints.h>
#include <SDL_video.h>

#include "graphic/Fast3D/gfx_metal.h"
#include <ImGui/backends/imgui_impl_metal.h>
#include <ImGui/backends/imgui_impl_sdl2.h>
#else
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_video.h>
#endif

#ifdef __SWITCH__
#include "port/switch/SwitchImpl.h"
#endif

#ifdef ENABLE_OPENGL
#include <ImGui/backends/imgui_impl_opengl3.h>
#include <ImGui/backends/imgui_impl_sdl2.h>

#endif

#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
#include <graphic/Fast3D/gfx_direct3d11.h>
#include <ImGui/backends/imgui_impl_dx11.h>
#include <ImGui/backends/imgui_impl_win32.h>

// NOLINTNEXTLINE
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

namespace LUS {
#define BindButton(btn, status)                                                                                    \
    ImGui::Image(GetTextureById(mGuiTextures[btn]->TextureId), ImVec2(16.0f * scale, 16.0f * scale), ImVec2(0, 0), \
                 ImVec2(1.0f, 1.0f), ImVec4(255, 255, 255, (status) ? 255 : 0));
#define TOGGLE_BTN ImGuiKey_F1
#define TOGGLE_PAD_BTN ImGuiKey_GamepadBack

Gui::Gui(std::shared_ptr<Window> window) : mWindow(window), mNeedsConsoleVariableSave(false) {
    mIsMenuShown = CVarGetInteger("gOpenMenuBar", 0);
    mGameOverlay = std::make_shared<GameOverlay>();
    mConsoleWindow = std::make_shared<ConsoleWindow>("Console");
    mInputEditorWindow = std::make_shared<InputEditorWindow>("Input Editor");
    mStatsWindow = std::make_shared<StatsWindow>("Stats");
}

void Gui::Init(WindowImpl windowImpl) {
    mImpl = windowImpl;
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);
    mImGuiIo = &ImGui::GetIO();
    mImGuiIo->ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NoMouseCursorChange;

    // Add Font Awesome and merge it into the default font.
    mImGuiIo->Fonts->AddFontDefault();
    // This must match the default font size, which is 13.0f.
    float baseFontSize = 13.0f;
    // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    float iconFontSize = baseFontSize * 2.0f / 3.0f;
    static const ImWchar sIconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode = true;
    iconsConfig.PixelSnapH = true;
    iconsConfig.GlyphMinAdvanceX = iconFontSize;
    mImGuiIo->Fonts->AddFontFromMemoryCompressedBase85TTF(fontawesome_compressed_data_base85, iconFontSize,
                                                          &iconsConfig, sIconsRanges);

#ifdef __SWITCH__
    LUS::Switch::ImGuiSetupFont(mImGuiIo->Fonts);
#endif

#ifdef __WIIU__
    // Scale everything by 2 for the Wii U
    ImGui::GetStyle().ScaleAllSizes(2.0f);
    mImGuiIo->FontGlobalScale = 2.0f;

    // Setup display sizes
    mImGuiIo->DisplaySize.x = mImpl.Gx2.Width;
    mImGuiIo->DisplaySize.y = mImpl.Gx2.Height;
#endif

    if (!IsMenuShown()) {
#if defined(__SWITCH__) || defined(__WIIU__)
        mGameOverlay->TextDrawNotification(30.0f, true, "Press - to access enhancements menu");
#else
        mGameOverlay->TextDrawNotification(30.0f, true, "Press F1 to access enhancements menu");
#endif
    }

    auto imguiIniPath = LUS::Context::GetPathRelativeToAppDirectory("imgui.ini");
    auto imguiLogPath = LUS::Context::GetPathRelativeToAppDirectory("imgui_log.txt");
    mImGuiIo->IniFilename = strcpy(new char[imguiIniPath.length() + 1], imguiIniPath.c_str());
    mImGuiIo->LogFilename = strcpy(new char[imguiLogPath.length() + 1], imguiLogPath.c_str());

    if (SupportsViewports() && CVarGetInteger("gEnableMultiViewports", 1)) {
        mImGuiIo->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    if (CVarGetInteger("gControlNav", 0) && IsMenuShown()) {
        mImGuiIo->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    } else {
        mImGuiIo->ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }

    GetConsoleWindow()->Init();
    GetGameOverlay()->Init();
    GetInputEditorWindow()->Init();

    ImGuiWMInit();
    ImGuiBackendInit();
#ifdef __SWITCH__
    ImGui::GetStyle().ScaleAllSizes(2);
#endif

    LUS::RegisterHook<LUS::GfxInit>([this] {
        if (Context::GetInstance()->GetWindow()->IsFullscreen()) {
            Context::GetInstance()->GetWindow()->SetCursorVisibility(IsMenuShown());
        }

        LoadTexture("Game_Icon", "textures/icons/gIcon.png");
        LoadTexture("A-Btn", "textures/buttons/ABtn.png");
        LoadTexture("B-Btn", "textures/buttons/BBtn.png");
        LoadTexture("L-Btn", "textures/buttons/LBtn.png");
        LoadTexture("R-Btn", "textures/buttons/RBtn.png");
        LoadTexture("Z-Btn", "textures/buttons/ZBtn.png");
        LoadTexture("Start-Btn", "textures/buttons/StartBtn.png");
        LoadTexture("C-Left", "textures/buttons/CLeft.png");
        LoadTexture("C-Right", "textures/buttons/CRight.png");
        LoadTexture("C-Up", "textures/buttons/CUp.png");
        LoadTexture("C-Down", "textures/buttons/CDown.png");
    });

    LUS::RegisterHook<LUS::ControllerRead>([this](OSContPad* contPads) { mPads = contPads; });

    InitSettings();

    CVarClear("gNewFileDropped");
    CVarClear("gDroppedFile");

#ifdef __SWITCH__
    Switch::ApplyOverclock();
#endif
}

void Gui::ImGuiWMInit() {
    switch (mImpl.RenderBackend) {
#ifdef __WIIU__
        case Backend::GX2:
            ImGui_ImplWiiU_Init();
            break;
#else
        case Backend::SDL_OPENGL:
            SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
            ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(mImpl.Opengl.Window), mImpl.Opengl.Context);
            break;
#endif
#if __APPLE__
        case Backend::SDL_METAL:
            SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
            ImGui_ImplSDL2_InitForMetal(static_cast<SDL_Window*>(mImpl.Metal.Window));
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case Backend::DX11:
            ImGui_ImplWin32_Init(mImpl.Dx11.Window);
            break;
#endif
        default:
            break;
    }
}

void Gui::ImGuiBackendInit() {
    switch (mImpl.RenderBackend) {
#ifdef __WIIU__
        case Backend::GX2:
            ImGui_ImplGX2_Init();
            break;
#else
        case Backend::SDL_OPENGL:
#ifdef __APPLE__
            ImGui_ImplOpenGL3_Init("#version 410 core");
#else
            ImGui_ImplOpenGL3_Init("#version 120");
#endif
            break;
#endif

#ifdef __APPLE__
        case Backend::SDL_METAL:
            Metal_Init(mImpl.Metal.Renderer);
            break;
#endif

#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case Backend::DX11:
            ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(mImpl.Dx11.Device),
                                static_cast<ID3D11DeviceContext*>(mImpl.Dx11.DeviceContext));
            break;
#endif
        default:
            break;
    }
}

void Gui::InitSettings() {
    LUS::RegisterHook<LUS::GfxInit>([this] {
        gfx_get_current_rendering_api()->set_texture_filter(
            (FilteringMode)CVarGetInteger("gTextureFilter", FILTER_THREE_POINT));

        GetConsoleWindow()->Init();
        GetInputEditorWindow()->Init();
        GetStatsWindow()->Init();
    });
}

void Gui::LoadTexture(const std::string& name, const std::string& path) {
    // TODO: Nothing ever unloads the texture from Fast3D here.
    GfxRenderingAPI* api = gfx_get_current_rendering_api();
    const auto res = Context::GetInstance()->GetResourceManager()->LoadFile(path);

    auto asset = GuiTexture(api->new_texture());
    uint8_t* imgData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(res->Buffer.data()), res->Buffer.size(),
                                             &asset.Width, &asset.Height, nullptr, 4);

    if (imgData == nullptr) {
        SPDLOG_ERROR("Error loading imgui texture {}", stbi_failure_reason());
        return;
    }

    api->select_texture(0, asset.TextureId);
    api->set_sampler_parameters(0, false, 0, 0);
    api->upload_texture(imgData, asset.Width, asset.Height);

    mGuiTextures[name] = asset;
    stbi_image_free(imgData);
}

bool Gui::SupportsViewports() {
#ifdef __SWITCH__
    return false;
#endif

    switch (mImpl.RenderBackend) {
        case Backend::DX11:
            return true;
        case Backend::SDL_OPENGL:
        case Backend::SDL_METAL:
            return true;
        default:
            return false;
    }
}

void Gui::Update(EventImpl event) {
    if (mNeedsConsoleVariableSave) {
        CVarSave();
        mNeedsConsoleVariableSave = false;
    }

    switch (mImpl.RenderBackend) {
#ifdef __WIIU__
        case Backend::GX2:
            if (!ImGui_ImplWiiU_ProcessInput((ImGui_ImplWiiU_ControllerInput*)event.Gx2.Input)) {}
            break;
#else
        case Backend::SDL_OPENGL:
        case Backend::SDL_METAL:
            ImGui_ImplSDL2_ProcessEvent(static_cast<const SDL_Event*>(event.Sdl.Event));

#ifdef __SWITCH__
            LUS::Switch::ImGuiProcessEvent(mImGuiIo->WantTextInput);
#endif
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case Backend::DX11:
            ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(event.Win32.Handle), event.Win32.Msg, event.Win32.Param1,
                                           event.Win32.Param2);
            break;
#endif
        default:
            break;
    }
}

void Gui::DrawMenu(void) {
    GetConsoleWindow()->Update();
    ImGuiBackendNewFrame();
    ImGuiWMNewFrame();
    ImGui::NewFrame();

    const std::shared_ptr<Window> wnd = Context::GetInstance()->GetWindow();
    const std::shared_ptr<Mercury> conf = Context::GetInstance()->GetConfig();

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                   ImGuiWindowFlags_NoResize;
    if (IsMenuShown()) {
        windowFlags |= ImGuiWindowFlags_MenuBar;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2((int)wnd->GetCurrentWidth(), (int)wnd->GetCurrentHeight()));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::Begin("Main - Deck", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    ImVec2 topLeftPos = ImGui::GetWindowPos();

    const ImGuiID dockId = ImGui::GetID("main_dock");

    if (!ImGui::DockBuilderGetNode(dockId)) {
        ImGui::DockBuilderRemoveNode(dockId);
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_NoTabBar);

        ImGui::DockBuilderDockWindow("Main Game", dockId);

        ImGui::DockBuilderFinish(dockId);
    }

    ImGui::DockSpace(dockId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_NoDockingInCentralNode);

    if (ImGui::IsKeyPressed(TOGGLE_BTN) || (ImGui::IsKeyPressed(TOGGLE_PAD_BTN) && CVarGetInteger("gControlNav", 0))) {
        mNeedsConsoleVariableSave = true;
        if (wnd->IsFullscreen()) {
            Context::GetInstance()->GetWindow()->SetCursorVisibility(IsMenuShown());
        }
        Context::GetInstance()->GetControlDeck()->SaveSettings();
        if (CVarGetInteger("gControlNav", 0) && IsMenuShown()) {
            mImGuiIo->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        } else {
            mImGuiIo->ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
        }
    }

#if __APPLE__
    if ((ImGui::IsKeyDown(ImGuiKey_LeftSuper) || ImGui::IsKeyDown(ImGuiKey_RightSuper)) &&
        ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        GetConsoleWindow()->Dispatch("reset");
    }
#else
    if ((ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) &&
        ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        GetConsoleWindow()->Dispatch("reset");
    }
#endif

    if (ImGui::BeginMenuBar()) {
        if (mGuiTextures.contains("Game_Icon")) {
#ifdef __SWITCH__
            ImVec2 iconSize = ImVec2(20.0f, 20.0f);
            float posScale = 1.0f;
#elif defined(__WIIU__)
            ImVec2 iconSize = ImVec2(16.0f * 2, 16.0f * 2);
            float posScale = 2.0f;
#else
            ImVec2 iconSize = ImVec2(16.0f, 16.0f);
            float posScale = 1.0f;
#endif
            ImGui::SetCursorPos(ImVec2(5, 2.5f) * posScale);
            ImGui::Image(GetTextureById(mGuiTextures["Game_Icon"]->TextureId), iconSize);
            ImGui::SameLine();
            ImGui::SetCursorPos(ImVec2(25, 0) * posScale);
        }

        static ImVec2 sWindowPadding(8.0f, 8.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, sWindowPadding);
        if (ImGui::BeginMenu("Shipwright")) {
            if (ImGui::MenuItem("Reset",
#ifdef __APPLE__
                                "Command-R"
#else
                                "Ctrl+R"
#endif
                                )) {
                GetConsoleWindow()->Dispatch("reset");
            }
#if !defined(__SWITCH__) && !defined(__WIIU__)
            const char* keyboardShortcut =
                strcmp(Context::GetInstance()->GetWindow()->GetWindowManagerName().c_str(), "sdl") == 0 ? "F10"
                                                                                                        : "ALT+Enter";
            if (ImGui::MenuItem("Toggle Fullscreen", keyboardShortcut)) {
                wnd->ToggleFullscreen();
            }
            if (ImGui::MenuItem("Quit")) {
                wnd->Close();
            }
#endif
            ImGui::EndMenu();
        }

        ImGui::SetCursorPosY(0.0f);

        ImGui::PopStyleVar(1);
        ImGui::EndMenuBar();
    }

    ImGui::End();

    GetStatsWindow()->Draw();
    GetConsoleWindow()->Draw();
    GetInputEditorWindow()->Draw();

    for (auto& windowIter : mGuiWindows) {
        windowIter.second->Update();

        if (windowIter.second->IsOpen()) {
            windowIter.second->Draw();
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("Main Game", nullptr, flags);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    ImVec2 mainPos = ImGui::GetWindowPos();
    mainPos.x -= topLeftPos.x;
    mainPos.y -= topLeftPos.y;
    ImVec2 size = ImGui::GetContentRegionAvail();
    gfx_current_dimensions.width = (uint32_t)(size.x * gfx_current_dimensions.internal_mul);
    gfx_current_dimensions.height = (uint32_t)(size.y * gfx_current_dimensions.internal_mul);
    gfx_current_game_window_viewport.x = (int16_t)mainPos.x;
    gfx_current_game_window_viewport.y = (int16_t)mainPos.y;
    gfx_current_game_window_viewport.width = (int16_t)size.x;
    gfx_current_game_window_viewport.height = (int16_t)size.y;

    switch (CVarGetInteger("gLowResMode", 0)) {
        case 1: { // N64 Mode
            gfx_current_dimensions.width = 320;
            gfx_current_dimensions.height = 240;
            const int sw = size.y * 320 / 240;
            gfx_current_game_window_viewport.x += ((int)size.x - sw) / 2;
            gfx_current_game_window_viewport.width = sw;
            break;
        }
        case 2: { // 240p Widescreen
            const int vertRes = 240;
            gfx_current_dimensions.width = vertRes * size.x / size.y;
            gfx_current_dimensions.height = vertRes;
            break;
        }
        case 3: { // 480p Widescreen
            const int vertRes = 480;
            gfx_current_dimensions.width = vertRes * size.x / size.y;
            gfx_current_dimensions.height = vertRes;
            break;
        }
    }

    GetGameOverlay()->Draw();
}

void Gui::ImGuiBackendNewFrame() {
    switch (mImpl.RenderBackend) {
#ifdef __WIIU__
        case Backend::GX2:
            mImGuiIo->DeltaTime = (float)frametime / 1000.0f / 1000.0f;
            ImGui_ImplGX2_NewFrame();
            break;
#else
        case Backend::SDL_OPENGL:
            ImGui_ImplOpenGL3_NewFrame();
            break;
#endif
#ifdef __APPLE__
        case Backend::SDL_METAL:
            Metal_NewFrame(mImpl.Metal.Renderer);
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case Backend::DX11:
            ImGui_ImplDX11_NewFrame();
            break;
#endif
        default:
            break;
    }
}

void Gui::ImGuiWMNewFrame() {
    switch (mImpl.RenderBackend) {
#ifdef __WIIU__
        case Backend::GX2:
            break;
#else
        case Backend::SDL_OPENGL:
        case Backend::SDL_METAL:
            ImGui_ImplSDL2_NewFrame();
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case Backend::DX11:
            ImGui_ImplWin32_NewFrame();
            break;
#endif
        default:
            break;
    }
}

void Gui::StartFrame() {
    const ImVec2 mainPos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImVec2 pos = ImVec2(0, 0);
    if (CVarGetInteger("gLowResMode", 0) == 1) {
        const float sw = size.y * 320.0f / 240.0f;
        pos = ImVec2(size.x / 2 - sw / 2, 0);
        size = ImVec2(sw, size.y);
    }
    if (gfxFramebuffer) {
        ImGui::SetCursorPos(pos);
        ImGui::Image(reinterpret_cast<ImTextureID>(gfxFramebuffer), size);
    }

    ImGui::End();

#ifdef __WIIU__
    const float scale = CVarGetFloat("gInputScale", 1.0f) * 2.0f;
#else
    const float scale = CVarGetFloat("gInputScale", 1.0f);
#endif

    ImVec2 btnPos = ImVec2(160 * scale, 85 * scale);

    if (CVarGetInteger("gInputEnabled", 0)) {
        ImGui::SetNextWindowSize(btnPos);
        ImGui::SetNextWindowPos(ImVec2(mainPos.x + size.x - btnPos.x - 20, mainPos.y + size.y - btnPos.y - 20));

        if (mPads != nullptr && ImGui::Begin("Game Buttons", nullptr,
                                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground)) {
            ImGui::SetCursorPosY(32 * scale);

            ImGui::BeginGroup();
            const ImVec2 cPos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(cPos.x + 10 * scale, cPos.y - 20 * scale));
            BindButton("L-Btn", mPads[0].button & BTN_L);
            ImGui::SetCursorPos(ImVec2(cPos.x + 16 * scale, cPos.y));
            BindButton("C-Up", mPads[0].button & BTN_CUP);
            ImGui::SetCursorPos(ImVec2(cPos.x, cPos.y + 16 * scale));
            BindButton("C-Left", mPads[0].button & BTN_CLEFT);
            ImGui::SetCursorPos(ImVec2(cPos.x + 32 * scale, cPos.y + 16 * scale));
            BindButton("C-Right", mPads[0].button & BTN_CRIGHT);
            ImGui::SetCursorPos(ImVec2(cPos.x + 16 * scale, cPos.y + 32 * scale));
            BindButton("C-Down", mPads[0].button & BTN_CDOWN);
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            const ImVec2 sPos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(sPos.x + 21, sPos.y - 20 * scale));
            BindButton("Z-Btn", mPads[0].button & BTN_Z);
            ImGui::SetCursorPos(ImVec2(sPos.x + 22, sPos.y + 16 * scale));
            BindButton("Start-Btn", mPads[0].button & BTN_START);
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            const ImVec2 bPos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(bPos.x + 20 * scale, bPos.y - 20 * scale));
            BindButton("R-Btn", mPads[0].button & BTN_R);
            ImGui::SetCursorPos(ImVec2(bPos.x + 12 * scale, bPos.y + 8 * scale));
            BindButton("B-Btn", mPads[0].button & BTN_B);
            ImGui::SetCursorPos(ImVec2(bPos.x + 28 * scale, bPos.y + 24 * scale));
            BindButton("A-Btn", mPads[0].button & BTN_A);
            ImGui::EndGroup();

            ImGui::End();
        }
    }

    ImGui::Render();
    ImGuiRenderDrawData(ImGui::GetDrawData());
    if (mImGuiIo->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        if ((mImpl.RenderBackend == Backend::SDL_OPENGL || mImpl.RenderBackend == Backend::SDL_METAL) &&
            mImpl.Opengl.Context != nullptr) {
            SDL_Window* backupCurrentWindow = SDL_GL_GetCurrentWindow();
            SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
        } else {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}

ImTextureID Gui::GetTextureById(int32_t id) {
#ifdef ENABLE_DX11
    if (mImpl.RenderBackend == Backend::DX11) {
        return gfx_d3d11_get_texture_by_id(id);
    }
#endif
#ifdef __APPLE__
    if (mImpl.RenderBackend == Backend::SDL_METAL) {
        return gfx_metal_get_texture_by_id(id);
    }
#endif
#ifdef __WIIU__
    if (mImpl.RenderBackend == Backend::GX2) {
        return gfx_gx2_texture_for_imgui(id);
    }
#endif

    return reinterpret_cast<ImTextureID>(id);
}

ImTextureID Gui::GetTextureByName(const std::string& name) {
    return GetTextureById(mGuiTextures[name]->TextureId);
}

void Gui::ImGuiRenderDrawData(ImDrawData* data) {
    switch (mImpl.RenderBackend) {
#ifdef __WIIU__
        case Backend::GX2:
            ImGui_ImplGX2_RenderDrawData(data);

            // Reset viewport and scissor for drawing the keyboard
            GX2SetViewport(0.0f, 0.0f, mImGuiIo->DisplaySize.x, mImGuiIo->DisplaySize.y, 0.0f, 1.0f);
            GX2SetScissor(0, 0, mImGuiIo->DisplaySize.x, mImGuiIo->DisplaySize.y);
            ImGui_ImplWiiU_DrawKeyboardOverlay();
            break;
#else
        case Backend::SDL_OPENGL:
            ImGui_ImplOpenGL3_RenderDrawData(data);
            break;
#endif
#ifdef __APPLE__
        case Backend::SDL_METAL:
            Metal_RenderDrawData(data);
            break;
#endif
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
        case Backend::DX11:
            ImGui_ImplDX11_RenderDrawData(data);
            break;
#endif
        default:
            break;
    }
}

void Gui::EndFrame() {
    ImGui::EndFrame();
    if (mImGuiIo->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
    }
}

void Gui::SaveConsoleVariablesOnNextTick() {
    mNeedsConsoleVariableSave = true;
}

void Gui::AddWindow(std::shared_ptr<GuiWindow> guiWindow) {
    if (mGuiWindows.contains(guiWindow->GetName())) {
        SPDLOG_ERROR("ImGui::AddWindow: Attempting to add duplicate window name {}", guiWindow->GetName());
        return;
    }

    mGuiWindows[guiWindow->GetName()] = guiWindow;
}

std::shared_ptr<GuiWindow> Gui::GetGuiWindow(const std::string& name) {
    return mGuiWindows[name];
}

void Gui::LoadGuiTexture(const std::string& name, const std::string& path, const ImVec4& tint) {
    GfxRenderingAPI* api = gfx_get_current_rendering_api();
    const auto res =
        static_cast<LUS::Texture*>(Context::GetInstance()->GetResourceManager()->LoadResource(path, true).get());

    std::vector<uint8_t> texBuffer;
    texBuffer.reserve(res->Width * res->Height * 4);

    switch (res->Type) {
        case LUS::TextureType::RGBA32bpp:
            texBuffer.assign(res->ImageData, res->ImageData + (res->Width * res->Height * 4));
            break;
        case LUS::TextureType::GrayscaleAlpha8bpp:
            for (int32_t i = 0; i < res->Width * res->Height; i++) {
                uint8_t ia = res->ImageData[i];
                uint8_t color = ((ia >> 4) & 0xF) * 255 / 15;
                uint8_t alpha = (ia & 0xF) * 255 / 15;
                texBuffer.push_back(color);
                texBuffer.push_back(color);
                texBuffer.push_back(color);
                texBuffer.push_back(alpha);
            }
            break;
        default:
            // TODO convert other image types
            SPDLOG_WARN("ImGui::LoadResource: Attempting to load unsupporting image type %s", path.c_str());
            return;
    }

    for (size_t pixel = 0; pixel < texBuffer.size() / 4; pixel++) {
        texBuffer[pixel * 4 + 0] *= tint.x;
        texBuffer[pixel * 4 + 1] *= tint.y;
        texBuffer[pixel * 4 + 2] *= tint.z;
        texBuffer[pixel * 4 + 3] *= tint.w;
    }

    auto asset = GuiTexture(api->new_texture());

    api->select_texture(0, asset.TextureId);
    api->set_sampler_parameters(0, false, 0, 0);
    api->upload_texture(texBuffer.data(), res->Width, res->Height);

    mGuiTextures[name] = asset;
}

void Gui::BeginGroupPanel(const char* name, const ImVec2& size) {
    ImGui::BeginGroup();

    // auto cursorPos = ImGui::GetCursorScreenPos();
    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f) {
        effectiveSize.x = ImGui::GetContentRegionAvail().x;
    } else {
        effectiveSize.x = size.x;
    }
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);
    auto labelMin = ImGui::GetItemRectMin();
    auto labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    ImGui::BeginGroup();

    // ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x -= frameHeight;

    auto itemWidth = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));
    mGroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}

void Gui::EndGroupPanel(float minHeight) {
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, std::max(frameHeight - frameHeight * 0.5f - itemSpacing.y, minHeight)));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    // ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

    auto labelRect = mGroupPanelLabelStack.back();
    mGroupPanelLabelStack.pop_back();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
    ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
    labelRect.Min.x -= itemSpacing.x;
    labelRect.Max.x += itemSpacing.x;
    for (int i = 0; i < 4; ++i) {
        switch (i) {
            // left half-plane
            case 0:
                ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true);
                break;
                // right half-plane
            case 1:
                ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true);
                break;
                // top
            case 2:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true);
                break;
                // bottom
            case 3:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true);
                break;
        }

        ImGui::GetWindowDrawList()->AddRect(frameRect.Min, frameRect.Max,
                                            ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), halfFrame.x);

        ImGui::PopClipRect();
    }

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::EndGroup();
}

std::shared_ptr<Window> Gui::GetWindow() {
    return mWindow;
}

std::shared_ptr<GameOverlay> Gui::GetGameOverlay() {
    return mGameOverlay;
}

std::shared_ptr<ConsoleWindow> Gui::GetConsoleWindow() {
    return mConsoleWindow;
}

std::shared_ptr<InputEditorWindow> Gui::GetInputEditorWindow() {
    return mInputEditorWindow;
}

std::shared_ptr<StatsWindow> Gui::GetStatsWindow() {
    return mStatsWindow;
}

bool Gui::IsMenuShown() {
    return mIsMenuShown;
}

void Gui::ShowMenu() {
    mIsMenuShown = true;
    CVarSetInteger("gOpenMenuBar", mIsMenuShown);
}

void Gui::HideMenu() {
    mIsMenuShown = false;
    CVarSetInteger("gOpenMenuBar", mIsMenuShown);
}
} // namespace LUS
