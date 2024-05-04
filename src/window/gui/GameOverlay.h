#pragma once
#include <string>
#include <vector>
#include <memory>

#include "debug/Console.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <unordered_map>

namespace Ship {

enum class OverlayType { TEXT, IMAGE, NOTIFICATION };

struct Overlay {
    OverlayType Type;
    std::string Value;
    float fadeTime;
    float duration;
};

class GameOverlay {
  public:
    GameOverlay();
    ~GameOverlay();

    void Init();
    void LoadFont(const std::string& name, const std::string& path, float fontSize);
    void SetCurrentFont(const std::string& name);
    void Draw();
    void DrawSettings();
    float GetStringWidth(const char* text);
    ImVec2 CalculateTextSize(const char* text, const char* textEnd = NULL, bool shortenText = false,
                             float wrapWidth = -1.0f);
    void TextDraw(float x, float y, bool shadow, ImVec4 color, const char* text, ...);
    void TextDrawNotification(float duration, bool shadow, const char* fmt, ...);
    void ClearNotifications();

  protected:
    float GetScreenWidth();
    float GetScreenHeight();

  private:
    std::unordered_map<std::string, ImFont*> mFonts;
    std::unordered_map<std::string, Overlay> mRegisteredOverlays;
    std::string mCurrentFont = "Default";
    bool mNeedsCleanup = false;

    void CleanupNotifications();
};
} // namespace Ship
