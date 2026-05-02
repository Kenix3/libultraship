#pragma once

#include <memory>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "ship/window/gui/Gui.h"
#include "ship/window/MouseStateManager.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/Component.h"

namespace Ship {

/** @brief Identifies the graphics/windowing backend in use. */
enum class WindowBackend { FAST3D_DXGI_DX11, FAST3D_SDL_OPENGL, FAST3D_SDL_METAL, WINDOW_BACKEND_COUNT };

/** @brief Integer pixel coordinates. */
struct Coords {
    int32_t x;
    int32_t y;
};

/** @brief Floating-point pixel coordinates. */
struct CoordsF {
    float x;
    float y;
};

class Config;

/**
 * @brief Abstract base class for the application window and rendering surface.
 *
 * Window owns the platform-specific window handle, the render loop, and the
 * ImGui/GUI layer. Concrete subclasses (e.g. an SDL+OpenGL window) implement the
 * pure virtual methods to integrate with specific graphics APIs.
 *
 * **Required Context children (looked up at construction time):**
 * - **Config** — fetched in the Window constructor to read and persist window
 *   settings (size, backend, fullscreen state). Config **must** be added to the
 *   Context before the Window is constructed.
 *
 * The window is added to Context as a child Component and is accessible via
 * `Context::GetChildren().GetFirst<Window>()`.
 */
class Window : public Component {
    friend class Context;

  public:
    Window();
    /**
     * @brief Constructs a Window and pre-registers a list of GUI windows.
     * @param guiWindows GUI overlay windows to register with the Gui layer.
     */
    Window(std::vector<std::shared_ptr<GuiWindow>> guiWindows);
    /**
     * @brief Constructs a Window backed by a pre-constructed Gui instance.
     * @param gui Gui layer to use.
     */
    Window(std::shared_ptr<Gui> gui);
    /**
     * @brief Constructs a Window with both a Gui and a MouseStateManager.
     * @param gui                Gui layer to use.
     * @param mouseStateManager  Mouse state tracker to use.
     */
    Window(std::shared_ptr<Gui> gui, std::shared_ptr<MouseStateManager> mouseStateManager);
    virtual ~Window();

    /** @brief Opens the window and initializes the graphics backend. */
    virtual void Init() = 0;
    /** @brief Requests that the window and application close. */
    virtual void Close() = 0;
    /** @brief Runs a single GUI-only tick without executing game logic. */
    virtual void RunGuiOnly() = 0;
    /** @brief Begins a new render frame; clears the back buffer and sets up state. */
    virtual void StartFrame() = 0;
    /** @brief Ends the current render frame and presents it to the display. */
    virtual void EndFrame() = 0;
    /** @brief Returns true if the renderer is ready to begin a new frame. */
    virtual bool IsFrameReady() = 0;
    /** @brief Processes pending OS/input events. */
    virtual void HandleEvents() = 0;
    /**
     * @brief Shows or hides the OS mouse cursor over the window.
     * @param visible true to show, false to hide.
     */
    virtual void SetCursorVisibility(bool visible) = 0;
    /** @brief Returns the current width of the window client area in pixels. */
    virtual uint32_t GetWidth() = 0;
    /** @brief Returns the current height of the window client area in pixels. */
    virtual uint32_t GetHeight() = 0;
    /** @brief Returns the current aspect ratio (width / height) of the window. */
    virtual float GetAspectRatio() = 0;
    /** @brief Returns the X position of the window's top-left corner on the desktop. */
    virtual int32_t GetPosX() = 0;
    /** @brief Returns the Y position of the window's top-left corner on the desktop. */
    virtual int32_t GetPosY() = 0;
    /**
     * @brief Moves the mouse cursor to the given window-relative coordinates.
     * @param pos Target position in window pixels.
     */
    virtual void SetMousePos(Coords pos) = 0;
    /** @brief Returns the current mouse cursor position relative to the window. */
    virtual Coords GetMousePos() = 0;
    /** @brief Returns the mouse movement delta since the last frame. */
    virtual Coords GetMouseDelta() = 0;
    /** @brief Returns the mouse wheel scroll delta for the current frame. */
    virtual CoordsF GetMouseWheel() = 0;
    /**
     * @brief Returns whether the given mouse button is currently pressed.
     * @param btn Mouse button to query.
     */
    virtual bool GetMouseState(MouseBtn btn) = 0;
    /**
     * @brief Enables or disables mouse capture (relative/locked mouse mode).
     * @param capture true to capture, false to release.
     */
    virtual void SetMouseCapture(bool capture) = 0;
    /** @brief Returns true if the mouse is currently captured. */
    virtual bool IsMouseCaptured() = 0;
    /** @brief Returns the display's current refresh rate in Hz. */
    virtual uint32_t GetCurrentRefreshRate() = 0;
    /** @brief Returns true if the backend supports a windowed-fullscreen (borderless) mode. */
    virtual bool SupportsWindowedFullscreen() = 0;
    /** @brief Returns true if vertical sync can be disabled on this backend. */
    virtual bool CanDisableVerticalSync() = 0;
    /**
     * @brief Sets the internal rendering resolution multiplier.
     * @param multiplier Values > 1 super-sample; < 1 sub-sample.
     */
    virtual void SetResolutionMultiplier(float multiplier) = 0;
    /**
     * @brief Sets the MSAA sample count.
     * @param value Number of MSAA samples (e.g. 1, 2, 4, 8).
     */
    virtual void SetMsaaLevel(uint32_t value) = 0;
    /**
     * @brief Switches between fullscreen and windowed mode.
     * @param isFullscreen true to enter fullscreen, false to return to windowed.
     */
    virtual void SetFullscreen(bool isFullscreen) = 0;
    /** @brief Returns true if the window is currently in fullscreen mode. */
    virtual bool IsFullscreen() = 0;
    /** @brief Returns true while the window / render loop is running. */
    virtual bool IsRunning() = 0;
    /**
     * @brief Returns the human-readable name for a keyboard scancode.
     * @param scancode Platform scancode value.
     * @return C-string name, or an empty string if unknown.
     */
    virtual const char* GetKeyName(int32_t scancode) = 0;
    /** @brief Returns a handle to the graphics API framebuffer object. */
    virtual uintptr_t GetGfxFrameBuffer() = 0;

    /** @brief Returns the current graphics backend identifier. */
    WindowBackend GetWindowBackend();
    /** @brief Returns the list of backends available on this platform. */
    std::shared_ptr<std::vector<WindowBackend>> GetAvailableWindowBackends();
    /**
     * @brief Returns true if the backend with the given ID is available on this platform.
     * @param backendId WindowBackend cast to int.
     */
    bool IsAvailableWindowBackend(int32_t backendId);
    /** @brief Returns the scancode of the last key event received. */
    int32_t GetLastScancode();
    /**
     * @brief Records the most recently received keyboard scancode.
     * @param scanCode Platform scancode value.
     */
    void SetLastScancode(int32_t scanCode);
    /** @brief Toggles between fullscreen and windowed mode. */
    void ToggleFullscreen();
    /** @brief Returns the current aspect ratio, recomputed from GetWidth() / GetHeight(). */
    float GetCurrentAspectRatio();
    /** @brief Persists window geometry and backend settings to the Config. */
    void SaveWindowToConfig();
    /** @brief Returns the GUI overlay layer. */
    std::shared_ptr<Gui> GetGui();
    /** @brief Returns true if the window should automatically capture the mouse when focused. */
    bool ShouldAutoCaptureMouse();
    /**
     * @brief Enables or disables automatic mouse capture on window focus.
     * @param capture true to enable auto-capture.
     */
    void SetAutoCaptureMouse(bool capture);
    /** @brief Returns true if the cursor should always be visible regardless of capture state. */
    bool ShouldForceCursorVisibility();
    /**
     * @brief Forces the cursor to remain visible even when captured.
     * @param visible true to force visibility.
     */
    void SetForceCursorVisibility(bool visible);
    /** @brief Returns the scancode bound to the fullscreen toggle action. */
    int32_t GetFullscreenScancode();
    /** @brief Returns the scancode bound to the mouse-capture toggle action. */
    int32_t GetMouseCaptureScancode();
    /**
     * @brief Binds a scancode to the fullscreen toggle action.
     * @param scancode Platform scancode.
     */
    void SetFullscreenScancode(int32_t scancode);
    /**
     * @brief Binds a scancode to the mouse-capture toggle action.
     * @param scancode Platform scancode.
     */
    void SetMouseCaptureScancode(int32_t scancode);
    /** @brief Returns the MouseStateManager used to track mouse button and position state. */
    std::shared_ptr<MouseStateManager> GetMouseStateManager();

  protected:
    /**
     * @brief Records the active graphics backend. Called by subclass constructors.
     * @param backend The backend in use.
     */
    void SetWindowBackend(WindowBackend backend);
    /**
     * @brief Adds a backend to the list of backends available on this platform.
     * @param backend Backend to mark as available.
     */
    void AddAvailableWindowBackend(WindowBackend backend);

  private:
    std::shared_ptr<Gui> mGui;
    int32_t mLastScancode = -1;
    WindowBackend mWindowBackend;
    std::shared_ptr<MouseStateManager> mMouseStateManager;
    std::shared_ptr<std::vector<WindowBackend>> mAvailableWindowBackends;
    // Hold a reference to Config because Window has a Save function called on Context destructor, where the singleton
    // is no longer available.
    std::shared_ptr<Config> mConfig;
    int32_t mFullscreenScancode;
    int32_t mMouseCaptureScancode;
};
} // namespace Ship
