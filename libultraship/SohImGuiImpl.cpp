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

#define USE_INTERNAL_RES

#define ImGuiWMInit() ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(impl.window), impl.context)
#define ImGuiBackendInit() ImGui_ImplOpenGL3_Init("#version 120")
#define ImGuiProcessEvent(event) ImGui_ImplSDL2_ProcessEvent(static_cast<const SDL_Event*>(event.handle))
#define ImGuiWMNewFrame() ImGui_ImplSDL2_NewFrame(static_cast<SDL_Window*>(impl.window))
#define ImGuiBackendNewFrame() ImGui_ImplOpenGL3_NewFrame()
#define ImGuiRenderDrawData(data) ImGui_ImplOpenGL3_RenderDrawData(data)

#elif defined(ENABLE_DX11) || defined(ENABLE_DX12)
#include "Lib/ImGui/backends/imgui_impl_dx11.h"
#include "Lib/ImGui/backends/imgui_impl_win32.h"
#include <Windows.h>

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define USE_VIEWPORTS

#define ImGuiWMInit() ImGui_ImplWin32_Init(impl.window)
#define ImGuiBackendInit() ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(impl.device), static_cast<ID3D11DeviceContext*>(impl.context))
#define ImGuiProcessEvent(event) ImGui_ImplWin32_WndProcHandler(static_cast<HWND>((event).handle), (event).msg, (event).wparam, (event).lparam)
#define ImGuiWMNewFrame() ImGui_ImplWin32_NewFrame()
#define ImGuiBackendNewFrame() ImGui_ImplDX11_NewFrame()
#define ImGuiRenderDrawData(data) ImGui_ImplDX11_RenderDrawData(data);
#endif

using namespace Ship;
SoHConfigType SohSettings;

#define TOGGLE_BTN ImGuiKey_F1
#define HOOK(b) if(b) needs_save = true;

namespace SohImGui {

    WindowImpl impl;
    ImGuiIO* io;
    Console* console = new Console;
    bool p_open = false;
    bool needs_save = false;

    void LoadSettings() {
        std::string ConfSection = GetDebugSection();
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();
        console->opened      = stob(Conf[ConfSection]["console"]);
        SohSettings.menu_bar = stob(Conf[ConfSection]["menu_bar"]);
        SohSettings.soh      = stob(Conf[ConfSection]["soh_debug"]);
#ifdef USE_INTERNAL_RES
        SohSettings.n64mode  = stob(Conf[ConfSection]["n64_mode"]);
#endif
    }

    void SaveSettings() {
        std::string ConfSection = GetDebugSection();
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();
        Conf[ConfSection]["console"]   = std::to_string(console->opened);
        Conf[ConfSection]["menu_bar"]  = std::to_string(SohSettings.menu_bar);
        Conf[ConfSection]["soh_debug"] = std::to_string(SohSettings.soh);
        Conf[ConfSection]["n64_mode"]  = std::to_string(SohSettings.n64mode);
        Conf.Save();
    }

    void Init( WindowImpl window_impl ) {
        LoadSettings();
        ImGuiContext* ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(ctx);
        io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#ifdef USE_VIEWPORTS
        io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
        impl = window_impl;
        console->Init();
        ImGuiWMInit();
        ImGuiBackendInit();
    }

    void Update(EventImpl event) {
        if(needs_save) {
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
#ifdef USE_VIEWPORTS
        window_flags |= ImGuiWindowFlags_NoBackground;
#endif
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
        }

        if (ImGui::BeginMenuBar()) {
            ImGui::Text("SoH Dev Menu");
            ImGui::Separator();
            if (ImGui::BeginMenu("View")) {
                HOOK(ImGui::MenuItem("Soh Debug", nullptr, &SohSettings.soh));
                HOOK(ImGui::MenuItem("Console", nullptr, &console->opened));
#ifdef USE_INTERNAL_RES
                HOOK(ImGui::MenuItem("N64 Mode", nullptr, &SohSettings.n64mode));
#endif
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
#ifdef USE_VIEWPORTS
        flags |= ImGuiWindowFlags_NoBackground;
#endif
        ImGui::Begin("OoT Master Quest", nullptr, flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImVec2 size = ImGui::GetContentRegionAvail();
        ImVec2 pos = ImVec2(0, 0);
        gfx_current_dimensions.width = size.x * gfx_current_dimensions.internal_mul;
        gfx_current_dimensions.height = size.y * gfx_current_dimensions.internal_mul;
#ifdef USE_INTERNAL_RES
        if (SohSettings.n64mode) {
            gfx_current_dimensions.width = 320;
            gfx_current_dimensions.height = 240;
            const int sw = size.y * 320 / 240;
            pos = ImVec2(size.x / 2 - sw / 2, 0);
            size = ImVec2(sw, size.y);
        }
#endif

#ifdef ENABLE_OPENGL
        int fbuf = std::stoi(SohUtils::getEnvironmentVar("framebuffer"));
        ImGui::ImageRotated(reinterpret_cast<ImTextureID>(fbuf), pos, size, 0.0f);
#endif
        ImGui::End();

        if (SohSettings.soh) {
	        const float framerate = ImGui::GetIO().Framerate;
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::Begin("Debug Stats", nullptr, ImGuiWindowFlags_None);

            ImGui::Text("Platform: Windows");
            ImGui::Text("Status: %.3f ms/frame (%.1f FPS)", 1000.0f / framerate, framerate);
#ifdef USE_INTERNAL_RES
            ImGui::Text("Internal Resolution:");
        	ImGui::SliderInt("Mul", reinterpret_cast<int*>(&gfx_current_dimensions.internal_mul), 1, 8);
#endif
            ImGui::End();
            ImGui::PopStyleColor();
        }

        console->Draw();

        ImGui::Render();
        ImGuiRenderDrawData(ImGui::GetDrawData());
#ifdef USE_VIEWPORTS
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
#endif
    }

    void BindCmd(const std::string& cmd, CommandEntry entry) {
        console->Commands[cmd] = std::move(entry);
    }

    std::string GetDebugSection() {
        return "DEBUG SETTINGS";
    }
}

extern "C" void c_init(WindowImpl impl) {
    SohImGui::Init(impl);
}
extern "C" void c_update(EventImpl event) {
    SohImGui::Update(event);
}
extern "C" void c_draw(void) {
    SohImGui::Draw();
}
