#pragma once

#ifdef __cplusplus

#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <SDL2/SDL.h>
#include "ship/window/gui/ConsoleWindow.h"
#include "ship/window/gui/InputEditorWindow.h"
#include "ship/controller/physicaldevice/SDLAddRemoveDeviceEventHandler.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/window/gui/GameOverlay.h"
#include "ship/window/gui/StatsWindow.h"
#include "ship/window/gui/GuiWindow.h"
#include "ship/window/gui/GuiMenuBar.h"

namespace Ship {
class Window;

/**
 * @brief Backend-specific data required to initialise the ImGui rendering context.
 *
 * The active union member is selected based on the graphics backend:
 * - Dx11   for DirectX 11 (Windows).
 * - Opengl for OpenGL (Linux/macOS via SDL).
 * - Metal  for Metal (macOS via SDL).
 * - Gx2    for the GX2 API (Wii U / Café OS).
 */
typedef struct {
    union {
        struct {
            void* Window;        ///< HWND
            void* DeviceContext; ///< ID3D11DeviceContext*
            void* Device;        ///< ID3D11Device*
        } Dx11;
        struct {
            void* Window;  ///< SDL_Window*
            void* Context; ///< SDL_GLContext
        } Opengl;
        struct {
            void* Window;           ///< SDL_Window*
            SDL_Renderer* Renderer; ///< SDL_Renderer* (for Metal layer)
        } Metal;
        struct {
            uint32_t Width;  ///< Framebuffer width in pixels.
            uint32_t Height; ///< Framebuffer height in pixels.
        } Gx2;
    };
} GuiWindowInitData;

/**
 * @brief Owns and drives the ImGui context, all registered GuiWindows, and texture management.
 *
 * Gui is the central hub for the in-game overlay system. It:
 * - Initialises the ImGui backend for the active graphics API.
 * - Maintains a registry of named GuiWindow instances and draws them each frame.
 * - Owns the GameOverlay, GuiMenuBar, and optional full-screen "menu" window.
 *
 * Obtain the instance from Window::GetGui().
 */
class Gui {
  public:
    /** @brief Constructs a Gui with no pre-registered windows. */
    Gui();

    /**
     * @brief Constructs a Gui and pre-registers a list of GuiWindows.
     * @param guiWindows Windows to add before Init() is called.
     */
    Gui(std::vector<std::shared_ptr<GuiWindow>> guiWindows);
    virtual ~Gui();

    /**
     * @brief Initialises the ImGui context and the appropriate backend renderer.
     * @param windowImpl Backend-specific handles required by the ImGui backend.
     */
    void Init(GuiWindowInitData windowImpl);

    /**
     * @brief Begins a new ImGui frame.
     *
     * Must be called once per frame before any ImGui draw calls. Calls
     * ImGui::NewFrame() after processing input via the backend.
     */
    void StartDraw();

    /**
     * @brief Finalises the ImGui frame and submits draw data to the renderer.
     *
     * Must be called once per frame after all ImGui draw calls.
     */
    void EndDraw();

    /**
     * @brief Schedules a CVar save to disk at the end of the current frame.
     *
     * Calling this multiple times in the same frame is safe — the save occurs only once.
     */
    void SaveConsoleVariablesNextFrame();

    /**
     * @brief Returns true if the ImGui multi-viewport / docking feature is supported
     * by the current backend.
     *
     * The base implementation returns false. Concrete subclasses (e.g. Fast3dGui)
     * override this to report actual backend capabilities.
     */
    virtual bool SupportsViewports();

    /**
     * @brief Returns the ImGui ID of the main game viewport window.
     *
     * Useful for docking other windows relative to the game view.
     */
    ImGuiID GetMainGameWindowID();

    /**
     * @brief Adds a GuiWindow to the draw loop.
     * @param guiWindow Window to register. Must have a unique name.
     */
    void AddGuiWindow(std::shared_ptr<GuiWindow> guiWindow);

    /**
     * @brief Returns the GuiWindow registered with the given name, or nullptr.
     * @param name Name passed to the GuiWindow constructor.
     */
    std::shared_ptr<GuiWindow> GetGuiWindow(const std::string& name);

    /**
     * @brief Removes a specific GuiWindow from the draw loop.
     * @param guiWindow Window to remove.
     */
    void RemoveGuiWindow(std::shared_ptr<GuiWindow> guiWindow);

    /**
     * @brief Removes the GuiWindow with the given name from the draw loop.
     * @param name Name of the window to remove.
     */
    void RemoveGuiWindow(const std::string& name);

    /** @brief Removes all registered GuiWindows from the draw loop. */
    void RemoveAllGuiWindows();

    /** @brief Returns the GameOverlay instance used for on-screen text and notifications. */
    std::shared_ptr<GameOverlay> GetGameOverlay();

    /**
     * @brief Sets the main application menu bar.
     * @param menuBar GuiMenuBar to render at the top of the screen each frame.
     */
    void SetMenuBar(std::shared_ptr<GuiMenuBar> menuBar);

    /** @brief Returns the registered menu bar, or nullptr if none has been set. */
    std::shared_ptr<GuiMenuBar> GetMenuBar();

    /**
     * @brief Sets an optional full-screen "menu" GuiWindow (e.g. a title-screen or pause menu).
     * @param menu GuiWindow to draw as the menu overlay.
     */
    void SetMenu(std::shared_ptr<GuiWindow> menu);

    /** @brief Returns the registered menu window, or nullptr if none has been set. */
    std::shared_ptr<GuiWindow> GetMenu();

    /**
     * @brief Returns true if the menu bar or the full-screen menu window is currently visible.
     */
    bool GetMenuOrMenubarVisible();

    /** @brief Returns true if the mouse cursor is currently over any ImGui item. */
    bool IsMouseOverAnyGuiItem();

    /** @brief Returns true if the mouse cursor is over an active ImGui popup. */
    bool IsMouseOverActivePopup();

    /** @brief Returns true if ImGui gamepad navigation is enabled. */
    bool GamepadNavigationEnabled();

    /** @brief Disables ImGui gamepad navigation (allows the game to use gamepad input). */
    void BlockGamepadNavigation();

    /** @brief Re-enables ImGui gamepad navigation. */
    void UnblockGamepadNavigation();

    /**
     * @brief Shuts down the ImGui context and releases backend resources.
     * @param window Pointer to the Window whose backend context should be torn down.
     */
    void ShutDownImGui(Ship::Window* window);

  protected:
    /** @brief Calls ImGui::NewFrame() after processing backend-specific input. */
    void StartFrame();

    /** @brief Calls ImGui::Render() and submits draw data to the backend. */
    void EndFrame();

    /** @brief Draws all registered floating GuiWindow instances.
     *  The base implementation is a no-op. Override in subclasses to provide viewport support. */
    virtual void DrawFloatingWindows();

    /** @brief Draws the menu bar and/or full-screen menu window. Override to add custom menus. */
    virtual void DrawMenu();

    /** @brief Renders the game viewport inside the main docking space.
     *  The base implementation is a no-op. Override in subclasses to render the game framebuffer. */
    virtual void DrawGame();

    /** @brief Recalculates the game viewport rect to account for the menu bar and window size.
     *  The base implementation is a no-op. Override in subclasses to set up the game viewport. */
    virtual void CalculateGameViewport();

    /** @brief Calls the appropriate ImGui backend New Frame function (DX11 / GL / Metal).
     *  The base implementation is a no-op. */
    virtual void ImGuiBackendNewFrame();

    /** @brief Calls ImGui_ImplSDL2_NewFrame() or the platform equivalent.
     *  The base implementation is a no-op. */
    virtual void ImGuiWMNewFrame();

    /** @brief Initialises the platform/window-manager ImGui backend.
     *  The base implementation is a no-op. */
    virtual void ImGuiWMInit();

    /** @brief Shuts down the platform/window-manager ImGui backend.
     *  The base implementation is a no-op. */
    virtual void ImGuiWMShutdown();

    /** @brief Initialises the renderer ImGui backend (DX11 / OpenGL / Metal).
     *  The base implementation is a no-op. */
    virtual void ImGuiBackendInit();

    /** @brief Shuts down the renderer ImGui backend.
     *  The base implementation is a no-op. */
    virtual void ImGuiBackendShutdown();

    /**
     * @brief Submits the ImGui draw data to the active graphics backend.
     * The base implementation is a no-op.
     * @param data Draw data produced by ImGui::Render().
     */
    virtual void ImGuiRenderDrawData(ImDrawData* data);

    /** @brief Flushes CVars to disk if SaveConsoleVariablesNextFrame() was called. */
    void CheckSaveCvars();

    /** @brief Updates mouse capture state based on window focus and UI interaction. */
    void HandleMouseCapture();

    ImVec2 mTemporaryWindowPos; ///< Scratchpad position used when repositioning windows.
    ImGuiIO* mImGuiIo;          ///< Pointer to the active ImGuiIO context.
    std::map<std::string, std::shared_ptr<GuiWindow>> mGuiWindows; ///< Registered window map (name → window).
    GuiWindowInitData mImpl; ///< Backend-specific window/context handles passed to Init().

  private:
    bool mNeedsConsoleVariableSave;
    std::string mImGuiIniPath;
    std::string mImGuiLogPath;
    std::shared_ptr<GameOverlay> mGameOverlay;
    std::shared_ptr<GuiMenuBar> mMenuBar;
    std::shared_ptr<GuiWindow> mMenu;
};
} // namespace Ship

#endif
