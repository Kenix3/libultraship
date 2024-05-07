#include "GameOverlay.h"

#include "public/bridge/consolevariablebridge.h"
#include "resource/File.h"
#include "window/gui/resource/Font.h"
#include "window/gui/resource/FontFactory.h"
#include "resource/archive/Archive.h"
#include "resource/ResourceManager.h"
#include "Context.h"
#include "utils/StringHelper.h"

namespace Ship {
GameOverlay::GameOverlay() {
}

GameOverlay::~GameOverlay() {
    SPDLOG_TRACE("destruct game overlay");
}

void GameOverlay::LoadFont(const std::string& name, const std::string& path, float fontSize) {
    ImGuiIO& io = ImGui::GetIO();
    auto initData = std::make_shared<ResourceInitData>();
    initData->Format = RESOURCE_FORMAT_BINARY;
    initData->Type = static_cast<uint32_t>(RESOURCE_TYPE_FONT);
    initData->ResourceVersion = 0;
    initData->Path = path;
    std::shared_ptr<Font> font = std::static_pointer_cast<Font>(
        Context::GetInstance()->GetResourceManager()->LoadResource(path, false, initData));

    if (font == nullptr) {
        SPDLOG_ERROR("Failed to load font: {}", name);
        return;
    }

    mFonts[name] = io.Fonts->AddFontFromMemoryTTF(font->Data, font->DataSize, fontSize);
}

void GameOverlay::TextDraw(float x, float y, bool shadow, ImVec4 color, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::PushFont(mFonts[mCurrentFont]);
    if (shadow) {
        ImGui::SetCursorPos(ImVec2(x + 1, y + 1));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.0f, .0f, .0f, color.w));
        ImGui::Text(buf, args);
    }
    ImGui::PopStyleColor();
    ImGui::SetCursorPos(ImVec2(x, y));
    ImGui::Text(buf, args);
    ImGui::PopFont();
    ImGui::PopStyleColor();
}

void GameOverlay::TextDrawNotification(float duration, bool shadow, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    mRegisteredOverlays[StringHelper::Sprintf("NotificationID:%d%d", rand(), mRegisteredOverlays.size())] =
        Overlay({ OverlayType::NOTIFICATION, buf, duration, duration });
    mNeedsCleanup = true;
}

void GameOverlay::ClearNotifications() {
    for (auto it = mRegisteredOverlays.begin(); it != mRegisteredOverlays.end();) {
        if (it->second.Type == OverlayType::NOTIFICATION) {
            it = mRegisteredOverlays.erase(it);
        } else {
            ++it;
        }
    }
}

void GameOverlay::CleanupNotifications() {
    if (!mNeedsCleanup) {
        return;
    }

    for (auto it = mRegisteredOverlays.begin(); it != mRegisteredOverlays.end();) {
        if (it->second.Type == OverlayType::NOTIFICATION && it->second.duration <= 0.0f) {
            it = mRegisteredOverlays.erase(it);
        } else {
            ++it;
        }
    }
    mNeedsCleanup = false;
}

float GameOverlay::GetScreenWidth() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    return viewport->Size.x;
}

float GameOverlay::GetScreenHeight() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    return viewport->Size.y;
}

float GameOverlay::GetStringWidth(const char* text) {
    return CalculateTextSize(text).x;
}

ImVec2 GameOverlay::CalculateTextSize(const char* text, const char* textEnd, bool shortenText, float wrapWidth) {
    ImGuiContext& g = *GImGui;

    const char* textDisplayEnd;
    if (shortenText) {
        textDisplayEnd = ImGui::FindRenderedTextEnd(text, textEnd); // Hide anything after a '##' string
    } else {
        textDisplayEnd = textEnd;
    }

    ImFont* font = mCurrentFont == "Default" ? g.Font : mFonts[mCurrentFont];
    const float fontSize = font->FontSize;
    if (text == textDisplayEnd) {
        return ImVec2(0.0f, fontSize);
    }
    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, wrapWidth, text, textDisplayEnd, NULL);

    // Round
    // FIXME: This has been here since Dec 2015 (7b0bf230) but down the line we want this out.
    // FIXME: Investigate using ceilf or e.g.
    // - https://git.musl-libc.org/cgit/musl/tree/src/math/ceilf.c
    // - https://embarkstudios.github.io/rust-gpu/api/src/libm/math/ceilf.rs.html
    textSize.x = IM_FLOOR(textSize.x + 0.99999f);

    return textSize;
}

void GameOverlay::Init() {
    Context::GetInstance()->GetResourceManager()->GetResourceLoader()->RegisterResourceFactory(
        std::make_shared<ResourceFactoryBinaryFontV0>(), RESOURCE_FORMAT_BINARY, "Font",
        static_cast<uint32_t>(RESOURCE_TYPE_FONT), 0);
}

void GameOverlay::SetCurrentFont(const std::string& name) {
    if (mFonts[name] == nullptr) {
        SPDLOG_ERROR("Failed to set current font: {}", name);
        return;
    }

    mCurrentFont = name;
    CVarSetString(CVAR_GAME_OVERLAY_FONT, name.c_str());
    Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesOnNextTick();
}

void GameOverlay::DrawSettings() {
    ImGui::Text("Overlays Text Font");
    if (ImGui::BeginCombo("##TextFont", mCurrentFont.c_str())) {
        for (auto& [name, font] : mFonts) {
            if (ImGui::Selectable(name.c_str(), name == mCurrentFont)) {
                SetCurrentFont(name);
            }
        }
        ImGui::EndCombo();
    }
}

void GameOverlay::Draw() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
    ImGui::Begin("GameOverlay", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoInputs);

    CleanupNotifications();

    float textY = 50;
    float notY = 0;

    for (auto& [key, overlay] : mRegisteredOverlays) {

        if (overlay.Type == OverlayType::TEXT) {
            const char* text = overlay.Value.c_str();
            const auto var = CVarGet(text);
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            switch (var->Type) {
                case ConsoleVariableType::Float:

                    TextDraw(30, textY, true, color, "%s %.2f", text, var->Float);
                    break;
                case ConsoleVariableType::Integer:
                    TextDraw(30, textY, true, color, "%s %d", text, var->Integer);
                    break;
                case ConsoleVariableType::String:
                    TextDraw(30, textY, true, color, "%s %s", text, var->String.c_str());
                    break;
                case ConsoleVariableType::Color:
                    TextDraw(30, textY, true, color, "%s (%u, %u, %u, %u)", text, var->Color.r, var->Color.g,
                             var->Color.b, var->Color.a);
                    break;
                case ConsoleVariableType::Color24:
                    TextDraw(30, textY, true, color, "%s (%u, %u, %u)", text, var->Color24.r, var->Color24.g,
                             var->Color24.b);
                    break;
            }

            free((void*)text);
            textY += 30;
        }

        if (overlay.Type == OverlayType::NOTIFICATION && overlay.duration > 0) {
            const char* text = overlay.Value.c_str();
            const float duration = overlay.duration / overlay.fadeTime;

            const ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, duration);
#ifdef __ANDROID__
            const float textWidth = GetStringWidth(overlay.Value.c_str()) * 2.0f;
            const float textOffset = 40.0f * 2.0f;
#else
            const float textWidth = GetStringWidth(overlay.Value.c_str());
            const float textOffset = 40.0f;
#endif

            TextDraw(GetScreenWidth() - textWidth - textOffset, GetScreenHeight() - textOffset - notY, true, color,
                     text);
            notY += 30;
            overlay.duration -= .05f;
        }
    }

    ImGui::End();
}
} // namespace Ship
