#pragma once
#include <string>
#include <vector>
#include <memory>

#include "ship/debug/Console.h"
#include <imgui.h>
#include <unordered_map>
#include "ship/resource/ResourceManager.h"

namespace Ship {

/** @brief Identifies the type of an overlay item rendered by GameOverlay. */
enum class OverlayType {
    TEXT,         ///< A fixed-position text label.
    IMAGE,        ///< An image drawn at a screen position.
    NOTIFICATION, ///< A transient notification that fades out after a duration.
};

/**
 * @brief An active overlay item managed by GameOverlay.
 */
struct Overlay {
    OverlayType Type;  ///< Discriminator for the overlay kind.
    std::string Value; ///< Text content or resource path, depending on Type.
    float fadeTime;    ///< Remaining fade-out time in seconds (notifications only).
    float duration;    ///< Total display duration in seconds (notifications only).
};

/**
 * @brief Renders on-screen timed notification messages.
 *
 * GameOverlay draws directly into the "game" ImGui viewport using a custom font
 * loaded from the archive. It is owned by Gui and is accessible via
 * Gui::GetGameOverlay().
 *
 * Fonts are loaded with LoadFont() and selected with SetCurrentFont(). Text can be
 * drawn at an arbitrary screen position with TextDraw(), or posted as a timed
 * notification with TextDrawNotification().
 */
class GameOverlay {
  public:
    GameOverlay();
    virtual ~GameOverlay();

    /** @brief Initialises the overlay and loads the default font. */
    void Init();

    /**
     * @brief Loads a font from an archive resource and registers it under @p name.
     * @param name       Cache key used by SetCurrentFont() and TextDraw().
     * @param fontSize   Point size.
     * @param identifier ResourceIdentifier of the font file within the archive.
     */
    void LoadFont(const std::string& name, float fontSize, const ResourceIdentifier& identifier);

    /**
     * @brief Loads a font from a virtual archive path and registers it under @p name.
     * @param name     Cache key.
     * @param fontSize Point size.
     * @param path     Virtual path of the font file within the archive.
     */
    void LoadFont(const std::string& name, float fontSize, const std::string& path);

    /**
     * @brief Selects the font used for subsequent TextDraw() and notification calls.
     * @param name Cache key registered via LoadFont().
     */
    void SetCurrentFont(const std::string& name);

    /** @brief Renders all active overlays for the current frame. Called by Gui::DrawGame(). */
    void Draw();

    /** @brief Renders the overlay settings panel (font selector, position, etc.) in ImGui. */
    void DrawSettings();

    /**
     * @brief Returns the pixel width of @p text using the currently selected font.
     * @param text Null-terminated string to measure.
     */
    float GetStringWidth(const char* text);

    /**
     * @brief Calculates the bounding box of the given text.
     * @param text        Text to measure.
     * @param textEnd     Optional end pointer (nullptr = measure until null terminator).
     * @param shortenText If true, the text may be clipped to @p wrapWidth.
     * @param wrapWidth   Maximum width before wrapping (-1 = no wrap).
     * @return Bounding size in pixels.
     */
    ImVec2 CalculateTextSize(const char* text, const char* textEnd = NULL, bool shortenText = false,
                             float wrapWidth = -1.0f);

    /**
     * @brief Draws @p text at the given screen position, optionally with a drop shadow.
     * @param x      Screen X coordinate in pixels.
     * @param y      Screen Y coordinate in pixels.
     * @param shadow If true, renders a black shadow one pixel behind the text.
     * @param color  RGBA colour of the text.
     * @param text   printf-style format string.
     */
    void TextDraw(float x, float y, bool shadow, ImVec4 color, const char* text, ...);

    /**
     * @brief Posts a timed notification message that fades out automatically.
     * @param duration Total display time in seconds.
     * @param shadow   If true, renders a drop shadow.
     * @param fmt      printf-style format string.
     */
    void TextDrawNotification(float duration, bool shadow, const char* fmt, ...);

    /** @brief Immediately removes all active notification overlays. */
    void ClearNotifications();

    /**
     * @brief Returns the ImGui ID of the overlay's invisible host window.
     *
     * Useful for docking or focus management.
     */
    ImGuiID GetID();

  protected:
    /** @brief Returns the current game viewport width in pixels. */
    float GetScreenWidth();
    /** @brief Returns the current game viewport height in pixels. */
    float GetScreenHeight();

  private:
    std::unordered_map<std::string, ImFont*> mFonts;
    std::unordered_map<std::string, Overlay> mRegisteredOverlays;
    std::string mCurrentFont = "Default";
    bool mNeedsCleanup = false;

    /** @brief Removes expired notification overlays from mRegisteredOverlays. */
    void CleanupNotifications();
};
} // namespace Ship
