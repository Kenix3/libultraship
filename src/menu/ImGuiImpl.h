#pragma once

#ifdef __cplusplus
#include "menu/GameOverlay.h"
#include <imgui.h>
#include <SDL2/SDL.h>
#include "menu/Console.h"
#include "InputEditor.h"
#include "menu/IconsFontAwesome4.h"

namespace LUS {
struct GameAsset {
    uint32_t textureId;
    int width;
    int height;
};

enum class Backend {
    DX11,
    SDL_OPENGL,
    SDL_METAL,
    GX2,
};

enum class Dialogues {
    Console,
    Menubar,
    LoadSettings,
};

typedef struct {
    Backend backend;
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

using WindowDrawFunc = void (*)(bool& enabled);

typedef struct {
    bool Enabled;
    WindowDrawFunc DrawFunc;
} CustomWindow;

bool SupportsWindowedFullscreen();
bool SupportsViewports();

void InitGui(WindowImpl windowImpl);
void UpdateGui(EventImpl event);

void DrawMainMenuAndCalculateGameSize(void);
void RegisterMenuDrawMethod(std::function<void(void)> drawMethod);
void AddSetupHooksDelegate(std::function<void(void)> setupHooksMethod);

void DrawFramebufferAndGameInput(void);
void RenderImGui(void);
void CancelFrame(void);
void DrawSettings();

// OTRTODO: Rename these two
uint32_t GetMenuBar();
void SetMenuBar(uint32_t menuBar);

Backend WindowBackend();
std::vector<std::pair<const char*, const char*>> GetAvailableRenderingBackends();
std::pair<const char*, const char*> GetCurrentRenderingBackend();
void SetCurrentRenderingBackend(uint8_t index, std::pair<const char*, const char*>);
const char** GetSupportedTextureFilters();
void SetResolutionMultiplier(float multiplier);
void SetMSAALevel(uint32_t value);

std::vector<std::pair<const char*, const char*>> GetAvailableAudioBackends();
std::pair<const char*, const char*> GetCurrentAudioBackend();
void SetCurrentAudioBackend(uint8_t index, std::pair<const char*, const char*>);

void AddWindow(const std::string& category, const std::string& name, WindowDrawFunc drawFunc, bool isEnabled = false,
               bool isHidden = false);
void EnableWindow(const std::string& name, bool isEnabled = true);

LUS::GameOverlay* GetGameOverlay();

LUS::InputEditor* GetInputEditor();
void ToggleInputEditorWindow(bool isOpen = true);
void ToggleStatisticsWindow(bool isOpen = true);

std::shared_ptr<LUS::Console> GetConsole();
void ToggleConsoleWindow(bool isOpen = true);
void DispatchConsoleCommand(const std::string& line);

void RequestCvarSaveOnNextTick();

ImTextureID GetTextureByID(int id);
ImTextureID GetTextureByName(const std::string& name);
void LoadResource(const std::string& name, const std::string& path, const ImVec4& tint = ImVec4(1, 1, 1, 1));

void setCursorVisibility(bool visible);
void BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f));
void EndGroupPanel(float minHeight = 0.0f);
} // namespace LUS

#endif
