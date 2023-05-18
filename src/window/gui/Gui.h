#pragma once

#ifdef __cplusplus
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <memory>
#include <SDL2/SDL.h>
#include "window/gui/ConsoleWindow.h"
#include "window/gui/InputEditorWindow.h"
#include "window/gui/IconsFontAwesome4.h"
#include "window/gui/GameOverlay.h"
#include "window/gui/StatsWindow.h"
#include "window/gui/GuiMenuBar.h"
#include "libultraship/libultra/controller.h"

namespace LUS {

typedef struct {
    union {
        struct {
            void* Window;
            void* DeviceContext;
            void* Device;
        } Dx11;
        struct {
            void* Window;
            void* Context;
        } Opengl;
        struct {
            void* Window;
            SDL_Renderer* Renderer;
        } Metal;
        struct {
            uint32_t Width;
            uint32_t Height;
        } Gx2;
    };
} GuiWindowInitData;

typedef union {
    struct {
        void* Handle;
        int Msg;
        int Param1;
        int Param2;
    } Win32;
    struct {
        void* Event;
    } Sdl;
    struct {
        void* Input;
    } Gx2;
} WindowEvent;

class Window;

class Gui {
  public:
    Gui(std::shared_ptr<Window> window);
    void Init(GuiWindowInitData windowImpl);
    void StartFrame();
    void EndFrame();
    void RenderViewports();
    void DrawMenu();

    void SaveConsoleVariablesOnNextTick();
    void Update(WindowEvent event);
    void AddGuiWindow(std::shared_ptr<GuiWindow> guiWindow);
    void RemoveGuiWindow(std::shared_ptr<GuiWindow> guiWindow);
    void RemoveGuiWindow(const std::string& name);
    void LoadGuiTexture(const std::string& name, const std::string& path, const ImVec4& tint);
    ImTextureID GetTextureByName(const std::string& name);
    bool SupportsViewports();
    void BeginGroupPanel(const char* name, const ImVec2& size);
    void EndGroupPanel(float minHeight);
    std::shared_ptr<GuiWindow> GetGuiWindow(const std::string& name);
    std::shared_ptr<Window> GetWindow();
    std::shared_ptr<GameOverlay> GetGameOverlay();
    void SetMenuBar(std::shared_ptr<GuiMenuBar> menuBar);
    std::shared_ptr<GuiMenuBar> GetMenuBar();

  protected:
    void InitSettings();
    void ImGuiWMInit();
    void ImGuiBackendInit();
    void LoadTexture(const std::string& name, const std::string& path);
    void ImGuiBackendNewFrame();
    void ImGuiWMNewFrame();
    void ImGuiRenderDrawData(ImDrawData* data);
    ImTextureID GetTextureById(int32_t id);

  private:
    struct GuiTexture {
        uint32_t RendererTextureId;
        int32_t Width;
        int32_t Height;
    };

    std::shared_ptr<Window> mWindow;
    OSContPad* mPads;
    GuiWindowInitData mImpl;
    ImGuiIO* mImGuiIo;
    bool mNeedsConsoleVariableSave;
    std::shared_ptr<GameOverlay> mGameOverlay;
    std::shared_ptr<GuiMenuBar> mMenuBar;
    std::map<std::string, GuiTexture> mGuiTextures;
    std::map<std::string, std::shared_ptr<GuiWindow>> mGuiWindows;
    ImVector<ImRect> mGroupPanelLabelStack;
};
} // namespace LUS

#endif
