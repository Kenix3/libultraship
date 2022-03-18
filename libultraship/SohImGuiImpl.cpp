#include "SohImGuiImpl.h"

#include <iostream>
#include <utility>

#include "Environment.h"
#include "SohConsole.h"
#include "SohHooks.h"
#include "Lib/ImGui/imgui_internal.h"
#include "GlobalCtx2.h"
#include "stox.h"
#include "Window.h"
#include "../Fast3D/gfx_pc.h"

#ifdef ENABLE_OPENGL
#include "Lib/ImGui/backends/imgui_impl_opengl3.h"
#include "Lib/ImGui/backends/imgui_impl_sdl.h"

#endif

#if defined(ENABLE_DX11) || defined(ENABLE_DX12)
#include "Lib/ImGui/backends/imgui_impl_dx11.h"
#include "Lib/ImGui/backends/imgui_impl_win32.h"
#include <Windows.h>

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
#include "Cvar.h"

using namespace Ship;
SoHConfigType SohSettings;

bool oldCursorState = true;


#define TOGGLE_BTN ImGuiKey_F1
#define HOOK(b) if(b) needs_save = true;

namespace SohImGui {

    WindowImpl impl;
    ImGuiIO* io;
    Console* console = new Console;
    bool p_open = false;
    bool needs_save = false;

    void ImGuiWMInit() {
        switch (impl.backend) {
        case Backend::SDL:
            ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(impl.sdl.window), impl.sdl.context);
            break;
        case Backend::DX11:
            ImGui_ImplWin32_Init(impl.dx11.window);
            break;
        }
    }

    void ImGuiBackendInit() {
        switch (impl.backend) {
        case Backend::SDL:
            ImGui_ImplOpenGL3_Init("#version 120");
            break;
        case Backend::DX11:
            ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(impl.dx11.device), static_cast<ID3D11DeviceContext*>(impl.dx11.device_context));
            break;
        }
    }

    void ImGuiProcessEvent(EventImpl event) {
        switch (impl.backend) {
        case Backend::SDL:
            ImGui_ImplSDL2_ProcessEvent(static_cast<const SDL_Event*>(event.sdl.event));
            break;
        case Backend::DX11:
            ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(event.win32.handle), event.win32.msg, event.win32.wparam, event.win32.lparam);
            break;
        }
    }

    void ImGuiWMNewFrame() {
        switch (impl.backend) {
        case Backend::SDL:
            ImGui_ImplSDL2_NewFrame(static_cast<SDL_Window*>(impl.sdl.window));
            break;
        case Backend::DX11:
            ImGui_ImplWin32_NewFrame();
            break;
        }
    }

    void ImGuiBackendNewFrame() {
        switch (impl.backend) {
        case Backend::SDL:
            ImGui_ImplOpenGL3_NewFrame();
            break;
        case Backend::DX11:
            ImGui_ImplDX11_NewFrame();
            break;
        }
    }

    void ImGuiRenderDrawData(ImDrawData* data) {
        switch (impl.backend) {
        case Backend::SDL:
            ImGui_ImplOpenGL3_RenderDrawData(data);
            break;
        case Backend::DX11:
            ImGui_ImplDX11_RenderDrawData(data);
            break;
        }
    }

    bool UseInternalRes() {
        switch (impl.backend) {
        case Backend::SDL:
            return true;
        }
        return false;
    }

    bool UseViewports() {
        switch (impl.backend) {
        case Backend::DX11:
            return true;
        }
        return false;
    }

    void SohImGui::ShowCursor(bool hide, Dialogues d) {
        if (d == Dialogues::dLoadSettings) {
            Ship::GlobalCtx2::GetInstance()->GetWindow()->ShowCursor(hide);
            return;
        }

        if (d == Dialogues::dConsole && SohSettings.menu_bar) {
            return;
        }
        if (!Ship::GlobalCtx2::GetInstance()->GetWindow()->IsFullscreen()) {
            oldCursorState = false;
            return;
        }

        if (oldCursorState != hide) {
            oldCursorState = hide;
            Ship::GlobalCtx2::GetInstance()->GetWindow()->ShowCursor(hide);
        }
    }

    void LoadSettings() {
        std::string ConfSection = GetDebugSection();
        std::string EnhancementSection = GetEnhancementSection();
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();

        // Debug
        console->opened = stob(Conf[ConfSection]["console"]);
        SohSettings.menu_bar = stob(Conf[ConfSection]["menu_bar"]);
        SohSettings.soh = stob(Conf[ConfSection]["soh_debug"]);

        if (UseInternalRes()) {
            SohSettings.n64mode = stob(Conf[ConfSection]["n64_mode"]);
        }

        // Enhancements
        SohSettings.fast_text = stob(Conf[EnhancementSection]["fast_text"]);
        CVar_SetS32((char*)"gFastText", SohSettings.fast_text);

        SohSettings.disable_lod = stob(Conf[EnhancementSection]["disable_lod"]);
        CVar_SetS32((char*)"gDisableLOD", SohSettings.disable_lod);

        SohSettings.animated_pause_menu = stob(Conf[EnhancementSection]["animated_pause_menu"]);
        CVar_SetS32((char*)"gPauseLiveLink", SohSettings.animated_pause_menu);

        SohSettings.debug_mode = stob(Conf[EnhancementSection]["debug_mode"]);
        CVar_SetS32((char*)"gDebugEnabled", SohSettings.debug_mode);
      
        // @bug DX ignores ShowCursor call if set here.
        if (impl.backend == Backend::SDL) {
            if (Ship::GlobalCtx2::GetInstance()->GetWindow()->IsFullscreen()) {
                if (SohSettings.menu_bar) {
                    SohImGui::ShowCursor(true, SohImGui::Dialogues::dLoadSettings);

                }
                else {
                    SohImGui::ShowCursor(false, SohImGui::Dialogues::dLoadSettings);
                }
            }
        }
    }

    void SaveSettings() {
        std::string ConfSection = GetDebugSection();
        std::string EnhancementSection = GetEnhancementSection();
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();

        // Debug
        Conf[ConfSection]["console"] = std::to_string(console->opened);
        Conf[ConfSection]["menu_bar"] = std::to_string(SohSettings.menu_bar);
        Conf[ConfSection]["soh_debug"] = std::to_string(SohSettings.soh);
        Conf[ConfSection]["n64_mode"] = std::to_string(SohSettings.n64mode);

        // Enhancements
        Conf[EnhancementSection]["fast_text"] = std::to_string(SohSettings.fast_text);
        Conf[EnhancementSection]["disable_lod"] = std::to_string(SohSettings.disable_lod);
        Conf[EnhancementSection]["animated_pause_menu"] = std::to_string(SohSettings.animated_pause_menu);
        Conf[EnhancementSection]["debug_mode"] = std::to_string(SohSettings.debug_mode);

        Conf.Save();
    }

    void Init(WindowImpl window_impl) {
        impl = window_impl;
        LoadSettings();
        ImGuiContext* ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(ctx);
        io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if (UseViewports()) {
            io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }
        console->Init();
        ImGuiWMInit();
        ImGuiBackendInit();
    }

    void Update(EventImpl event) {
        if (needs_save) {
            SaveSettings();
            needs_save = false;
        }
        ImGuiProcessEvent(event);
    }

    void Draw(void) {
        console->Update();
        ImGuiBackendNewFrame();
        ImGuiWMNewFrame();
        ImGui::NewFrame();

        const std::shared_ptr<Window> wnd = GlobalCtx2::GetInstance()->GetWindow();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoResize;
        if (UseViewports()) {
            window_flags |= ImGuiWindowFlags_NoBackground;
        }
        if (SohSettings.menu_bar) window_flags |= ImGuiWindowFlags_MenuBar;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(ImVec2(wnd->GetCurrentWidth(), wnd->GetCurrentHeight()));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Main - Deck", nullptr, window_flags);
        ImGui::PopStyleVar();

        const ImGuiID dockId = ImGui::GetID("main_dock");

        if (!ImGui::DockBuilderGetNode(dockId)) {
            ImGui::DockBuilderRemoveNode(dockId);
            ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_NoTabBar);

            ImGui::DockBuilderDockWindow("OoT Master Quest", dockId);

            ImGui::DockBuilderFinish(dockId);
        }

        ImGui::DockSpace(dockId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        if (ImGui::IsKeyPressed(TOGGLE_BTN)) {
            SohSettings.menu_bar = !SohSettings.menu_bar;
            needs_save = true;
            GlobalCtx2::GetInstance()->GetWindow()->dwMenubar = SohSettings.menu_bar;
            SohImGui::ShowCursor(SohSettings.menu_bar, Dialogues::dMenubar);
        }

        if (ImGui::BeginMenuBar()) {
            ImGui::Text("SoH Dev Menu");
            ImGui::Separator();

            if (ImGui::BeginMenu("Enhancements")) {

                ImGui::Text("Gameplay");
                ImGui::Separator();

                if (ImGui::Checkbox("Fast Text", &SohSettings.fast_text)) {
                    CVar_SetS32((char*)"gFastText", SohSettings.fast_text);
                    needs_save = true;
                }

                ImGui::Text("Graphics");
                ImGui::Separator();

                if (ImGui::Checkbox("Animated Link in Pause Menu", &SohSettings.animated_pause_menu)) {
                    CVar_SetS32((char*)"gPauseLiveLink", SohSettings.animated_pause_menu);
                    needs_save = true;
                }

                if (ImGui::Checkbox("Disable LOD", &SohSettings.disable_lod)) {
                    CVar_SetS32((char*)"gDisableLOD", SohSettings.disable_lod);
                    needs_save = true;
                }

                ImGui::Text("Debugging");
                ImGui::Separator();

                if (ImGui::Checkbox("Debug Mode", &SohSettings.debug_mode)) {
                    CVar_SetS32((char*)"gDebugEnabled", SohSettings.debug_mode);
                    needs_save = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Developer Tools")) {
                HOOK(ImGui::MenuItem("Stats", nullptr, &SohSettings.soh));
                HOOK(ImGui::MenuItem("Console", nullptr, &console->opened));
                if (UseInternalRes()) {
                    HOOK(ImGui::MenuItem("N64 Mode", nullptr, &SohSettings.n64mode));
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        if (UseViewports()) {
            flags |= ImGuiWindowFlags_NoBackground;
        }
        ImGui::Begin("OoT Master Quest", nullptr, flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImVec2 size = ImGui::GetContentRegionAvail();
        ImVec2 pos = ImVec2(0, 0);
        gfx_current_dimensions.width = size.x * gfx_current_dimensions.internal_mul;
        gfx_current_dimensions.height = size.y * gfx_current_dimensions.internal_mul;
        if (UseInternalRes()) {
            if (SohSettings.n64mode) {
                gfx_current_dimensions.width = 320;
                gfx_current_dimensions.height = 240;
                const int sw = size.y * 320 / 240;
                pos = ImVec2(size.x / 2 - sw / 2, 0);
                size = ImVec2(sw, size.y);
            }
        }

        if (UseInternalRes()) {
            int fbuf = std::stoi(SohUtils::getEnvironmentVar("framebuffer"));
            ImGui::ImageRotated(reinterpret_cast<ImTextureID>(fbuf), pos, size, 0.0f);
        }
        ImGui::End();

        if (SohSettings.soh) {
            const float framerate = ImGui::GetIO().Framerate;
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::Begin("Debug Stats", nullptr, ImGuiWindowFlags_None);

            ImGui::Text("Platform: Windows");
            ImGui::Text("Status: %.3f ms/frame (%.1f FPS)", 1000.0f / framerate, framerate);
            if (UseInternalRes()) {
                ImGui::Text("Internal Resolution:");
                ImGui::SliderInt("Mul", reinterpret_cast<int*>(&gfx_current_dimensions.internal_mul), 1, 8);
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        console->Draw();

        ImGui::Render();
        ImGuiRenderDrawData(ImGui::GetDrawData());
        if (UseViewports()) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void BindCmd(const std::string& cmd, CommandEntry entry) {
        console->Commands[cmd] = std::move(entry);
    }

    std::string GetDebugSection() {
        return "DEBUG SETTINGS";
    }

    std::string GetEnhancementSection() {
        return "ENHANCEMENT SETTINGS";
    }
}