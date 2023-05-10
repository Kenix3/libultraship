#pragma once
#include <string>
#include <vector>
#include <memory>

#include "menu/Console.h"
#include <imgui.h>
#include <unordered_map>

namespace LUS {

enum class OverlayType { TEXT, IMAGE, NOTIFICATION };

struct Overlay {
    OverlayType type;
    const char* value;
    float fadeTime;
    float duration;
};

class GameOverlay {
  public:
    static bool OverlayCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args);

    void Init();
    void Draw();
    void DrawSettings();
    static float GetStringWidth(const char* text);
    static ImVec2 CalculateTextSize(const char* text, const char* textEnd = NULL, bool shortenText = false,
                                    float wrapWidth = -1.0f);
    void TextDraw(float x, float y, bool shadow, ImVec4 color, const char* text, ...);
    void TextDrawNotification(float duration, bool shadow, const char* fmt, ...);
    void ClearNotifications();

  protected:
    static float GetScreenWidth();
    static float GetScreenHeight();

  private:
    std::unordered_map<std::string, ImFont*> Fonts;
    std::unordered_map<std::string, Overlay*> RegisteredOverlays;
    std::string CurrentFont = "Default";
    bool NeedsCleanup = false;

    void CleanupNotifications();
    void LoadFont(const std::string& name, const std::string& path, float fontSize);
};
} // namespace LUS
