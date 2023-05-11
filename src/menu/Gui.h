#pragma once

#ifdef __cplusplus
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <memory>
#include <SDL2/SDL.h>
#include "menu/ConsoleWindow.h"
#include "menu/InputEditorWindow.h"
#include "menu/IconsFontAwesome4.h"
#include "menu/GameOverlay.h"
#include "menu/StatsWindow.h"
#include "menu/GuiMenuBar.h"
#include "libultraship/libultra/controller.h"

namespace LUS {
enum class Backend {
    DX11,
    SDL_OPENGL,
    SDL_METAL,
    GX2,
};

typedef struct {
    Backend RenderBackend;
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
} WindowImpl;

struct GuiTexture {
    uint32_t TextureId;
    int32_t Width;
    int32_t Height;
};

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
} EventImpl;

class Window;

class Gui {
  public:
    Gui(std::shared_ptr<Window> window);
    void Init(WindowImpl windowImpl);
    void StartFrame();
    void EndFrame();

    void SaveConsoleVariablesOnNextTick();
    void DrawMenu(void);
    void Update(EventImpl event);
    void AddWindow(std::shared_ptr<GuiWindow> guiWindow);
    void LoadGuiTexture(const std::string& name, const std::string& path, const ImVec4& tint);
    ImTextureID GetTextureById(int32_t id);
    ImTextureID GetTextureByName(const std::string& name);
    std::map<std::string, GuiTexture> GetGuiTextures();
    bool SupportsViewports();
    void BeginGroupPanel(const char* name, const ImVec2& size);
    void EndGroupPanel(float minHeight);
    std::shared_ptr<GuiWindow> GetGuiWindow(const std::string& name);
    std::shared_ptr<Window> GetWindow();
    std::shared_ptr<ConsoleWindow> GetConsoleWindow();
    std::shared_ptr<GameOverlay> GetGameOverlay();
    void SetMenuBar(std::shared_ptr<GuiMenuBar> menuBar);
    std::shared_ptr<GuiMenuBar> GetMenuBar();
    bool IsMenuShown();
    void ShowMenu();
    void HideMenu();
    Backend GetRenderBackend();
    std::shared_ptr<InputEditorWindow> GetInputEditorWindow();

  protected:
    void InitSettings();
    void ImGuiWMInit();
    void ImGuiBackendInit();
    void LoadTexture(const std::string& name, const std::string& path);
    void ImGuiBackendNewFrame();
    void ImGuiWMNewFrame();
    void ImGuiRenderDrawData(ImDrawData* data);

    std::shared_ptr<StatsWindow> GetStatsWindow();

  private:
    std::shared_ptr<Window> mWindow;
    OSContPad* mPads;
    WindowImpl mImpl;
    ImGuiIO* mImGuiIo;
    bool mNeedsConsoleVariableSave;
    bool mIsMenuShown;
    std::shared_ptr<GameOverlay> mGameOverlay;
    std::shared_ptr<ConsoleWindow> mConsoleWindow;
    std::shared_ptr<InputEditorWindow> mInputEditorWindow;
    std::shared_ptr<StatsWindow> mStatsWindow;
    std::shared_ptr<GuiMenuBar> mMenuBar;
    std::map<std::string, GuiTexture> mGuiTextures;
    std::map<std::string, std::shared_ptr<GuiWindow>> mGuiWindows;
    ImVector<ImRect> mGroupPanelLabelStack;
};
} // namespace LUS

#endif
