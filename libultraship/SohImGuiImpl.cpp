#include "SohImGuiImpl.h"

#include <utility>

#ifdef ENABLE_OPENGL
#include "Lib/ImGui/backends/imgui_impl_opengl3.h"
#include "Lib/ImGui/backends/imgui_impl_sdl.h"

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

#define ImGuiWMInit() ImGui_ImplWin32_Init(impl.window);
#define ImGuiBackendInit() ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(impl.device), static_cast<ID3D11DeviceContext*>(impl.context));
#define ImGuiProcessEvent(event) ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(event.handle), event.msg, event.wparam, event.lparam)
#define ImGuiWMNewFrame() ImGui_ImplWin32_NewFrame()
#define ImGuiBackendNewFrame() ImGui_ImplDX11_NewFrame()
#define ImGuiRenderDrawData(data) ImGui_ImplDX11_RenderDrawData(data);
#endif


namespace SohImGui {

    WindowImpl impl;
    ImGuiIO* io;
    SohConsole* console;
    bool p_open = true;

    void init( WindowImpl window_impl ) {
        ImGuiContext* ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(ctx);
        io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        impl = window_impl;
        console = new SohConsole();
        ImGuiWMInit();
        ImGuiBackendInit();
    }

    void update(EventImpl event) {
        ImGuiProcessEvent(event);
    }

    void draw(void) {
        ImGuiBackendNewFrame();
        ImGuiWMNewFrame();
        ImGui::NewFrame();

        ImGui::Begin("SOH Debug");
			ImGui::Text("Platform: Windows");
        ImGui::End();

        console->Draw("SOH Console", &p_open);

        ImGui::Render();
        ImGuiRenderDrawData(ImGui::GetDrawData());
    }

    void BindCmd(const std::string cmd, CommandFunc func) {
        Commands[cmd] = std::move(func);
    }
}

extern "C" void c_init(WindowImpl impl) {
    SohImGui::init(impl);
}
extern "C" void c_update(EventImpl event) {
    SohImGui::update(event);
}
extern "C" void c_draw(void) {
    SohImGui::draw();
}
