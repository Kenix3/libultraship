#pragma once

#include <memory>

namespace Ship {

/**
 * @brief Manages mouse capture and cursor visibility state for the application window.
 *
 * MouseStateManager handles auto-capturing the mouse when the window has focus,
 * forcing cursor visibility (e.g. when GUI menus are open), and implementing a
 * timed cursor-hide after a period of inactivity.
 */
class MouseStateManager {
  public:
    /** @brief Constructs a MouseStateManager with default settings. */
    MouseStateManager();

    /** @brief Virtual destructor. */
    virtual ~MouseStateManager();

    /** @brief Called at the beginning of each frame to update cursor visibility and capture state. */
    virtual void StartFrame();

    /**
     * @brief Checks whether the mouse should be automatically captured by the window.
     * @return true if auto-capture is enabled.
     */
    virtual bool ShouldAutoCaptureMouse();

    /**
     * @brief Enables or disables automatic mouse capture.
     * @param capture true to enable auto-capture, false to disable.
     */
    virtual void SetAutoCaptureMouse(bool capture);

    /**
     * @brief Checks whether cursor visibility is being forced on.
     * @return true if cursor visibility is forced.
     */
    virtual bool ShouldForceCursorVisibility();

    /**
     * @brief Forces the cursor to remain visible regardless of other state.
     * @param visible true to force cursor visibility, false to allow normal behavior.
     */
    virtual void SetForceCursorVisibility(bool visible);

    /** @brief Toggles the manual mouse capture override on or off. */
    virtual void ToggleMouseCaptureOverride();

    /** @brief Applies the current mouse capture settings to the underlying window system. */
    virtual void UpdateMouseCapture();

    /** @brief Resets the cursor visibility timeout counter to its configured value. */
    virtual void ResetCursorVisibilityTimer();

    /**
     * @brief Sets the number of ticks before the cursor is automatically hidden.
     * @param ticks The visibility duration in ticks.
     */
    void SetCursorVisibilityTimeTicks(uint32_t ticks);

    /**
     * @brief Returns the configured cursor visibility timeout in ticks.
     * @return The number of ticks before the cursor hides.
     */
    uint32_t GetCursorVisibilityTimeTicks();

  protected:
    /** @brief Decrements the cursor visibility timeout counter by one tick. */
    void CursorVisibilityTimeoutTick();

  private:
    bool mAutoCaptureMouse = false;
    bool mForceCursorVisibility = false;

    uint32_t mCursorVisibleTicks = 180;
    uint32_t mCursorVisibleTicksCounter = mCursorVisibleTicks;
};
} // namespace Ship
