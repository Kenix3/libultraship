#include "ship/window/gui/OnScreenKeyboard.h"

#include <imgui_internal.h>

#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"
#include "ship/window/gui/IconsFontAwesome4.h"

namespace Ship {
namespace {

constexpr float gMinRowWidth = 10.0f;
constexpr float gHoldSeconds = 0.35f;
constexpr ImGuiID gKeyOwner =
    0x4F534B; // ASCII "OSK"; here so the open text field doesn't read A as validate and B as cancel.
constexpr int gNumericFlags = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal |
                              ImGuiInputTextFlags_CharsScientific | ImGuiInputTextFlags_LocalizeDecimalPoint;

struct Language {
    const char* Code;
    const char* Name;
    const char16_t* Rows[3];
    char16_t Currency;
};

struct Variants {
    char16_t Base;
    const char16_t* Characters;
};

constexpr Language gLanguages[] = {
    { "EN", "English (US)", { u"qwertyuiop", u"asdfghjkl", u"zxcvbnm" }, u'$' },
    { "EN", "English (UK)", { u"qwertyuiop", u"asdfghjkl", u"zxcvbnm" }, u'£' },
    { "FR", "Français", { u"azertyuiop", u"qsdfghjklm", u"wxcvbn'" }, u'€' },
    { "DE", "Deutsch", { u"qwertzuiopü", u"asdfghjklöä", u"yxcvbnmß" }, u'€' },
    { "ES", "Español", { u"qwertyuiop", u"asdfghjklñ", u"zxcvbnm" }, u'€' },
};
constexpr int gLanguageCount = IM_ARRAYSIZE(gLanguages);

constexpr const char16_t* gSymbolRows[2][3] = {
    { u"1234567890", u"@#$_&-+()/", u"*\"':;!?" },
    { u"~`|•°×÷¶§±", u"£¢€¥$^={}\\", u"%©®[]<>" },
};

// Shown when the type button is held on a key
constexpr Variants gVariants[] = {
    { u'a', u"àáâäãåæāăą" }, { u'c', u"çćĉċč" },      { u'd', u"ðďđ" }, { u'e', u"éèêëēĕėęě" }, { u'g', u"ĝğġģ" },
    { u'h', u"ĥħ" },         { u'i', u"íìîïĩīĭį" },   { u'j', u"ĵ" },   { u'k', u"ķ" },         { u'l', u"ĺļľŀł" },
    { u'n', u"ñńņňŋ" },      { u'o', u"óòôöõøœōŏő" }, { u'r', u"ŕŗř" }, { u's', u"ßśŝşš" },     { u't', u"þţťŧ" },
    { u'u', u"úùûüũūŭůűų" }, { u'w', u"ŵ" },          { u'y', u"ýÿŷ" }, { u'z', u"źżž" },       { u'?', u"¿" },
    { u'!', u"¡" },          { u'"', u"«»“”" },       { u'\'', u"‘’" }, { u'-', u"–—" },        { u'.', u"…" },
};

unsigned int ToUpper(unsigned int c) {
    if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7)) {
        return c - 0x20;
    }
    if (c == 0xFF) {
        return 0x178;
    }
    // Latin Extended-A alternates case. Upper case is on even code points in the first blocks, odd in the second.
    const bool upperIsEven = (c >= 0x100 && c <= 0x137) || (c >= 0x14A && c <= 0x177);
    const bool upperIsOdd = (c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E);
    return ((upperIsEven && (c & 1)) || (upperIsOdd && !(c & 1))) ? c - 1 : c;
}

void TapKey(ImGuiKey key) {
    ImGui::GetIO().AddKeyEvent(key, true);
    ImGui::GetIO().AddKeyEvent(key, false);
}

void Finish(ImGuiContext& g) {
    // Enter is a newline in a multiline field
    if (g.InputTextState.Flags & ImGuiInputTextFlags_Multiline) {
        ImGui::ClearActiveID();
    } else {
        TapKey(ImGuiKey_Enter);
    }
}

bool AnyPressed(const std::vector<ImGuiKey>& keys, bool repeat = true) {
    for (ImGuiKey key : keys) {
        if (ImGui::IsKeyPressed(key, repeat)) {
            return true;
        }
    }
    return false;
}

// Negative when none of the keys are down
float HeldSeconds(const std::vector<ImGuiKey>& keys) {
    float held = -1.0f;
    for (ImGuiKey key : keys) {
        held = ImMax(held, ImGui::GetKeyData(key)->DownDuration);
    }
    return held;
}

std::string ToUtf8(unsigned int c) {
    char buffer[5];
    ImTextCharToUtf8(buffer, c);
    return buffer;
}

void DrawCentered(ImDrawList* drawList, ImFont* font, float size, const ImVec2& min, const ImVec2& max, ImU32 color,
                  const std::string& text) {
    const ImVec2 textSize = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str());
    drawList->AddText(font, size, min + (max - min - textSize) / 2, color, text.c_str());
}

void DrawPreview(ImGuiContext& g, ImDrawList* drawList, ImFont* font, float size, const ImVec2& min, const ImVec2& max,
                 ImU32 color) {
    ImGuiInputTextState& state = g.InputTextState;
    std::string text;
    if (state.TextLen > 0) {
        text.assign(state.TextA.Data, state.TextLen);
    }
    int caret = ImMin(state.GetCursorPos(), state.TextLen);
    if (state.Flags & ImGuiInputTextFlags_Password) {
        const char* begin = text.c_str();
        const int length = ImTextCountCharsFromUtf8(begin, begin + text.size());
        caret = ImTextCountCharsFromUtf8(begin, begin + caret);
        text.assign(length, '*');
    }

    const float inset = size / 3;
    const float caretOffset = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str(), text.c_str() + caret).x;
    const float scroll = ImMax(0.0f, caretOffset - (max.x - min.x - inset * 2));
    const ImVec2 textPos(min.x + inset - scroll, min.y + (max.y - min.y - size) / 2);

    drawList->PushClipRect(min, max, true);
    drawList->AddText(font, size, textPos, color, text.c_str());
    drawList->AddLine(ImVec2(textPos.x + caretOffset, textPos.y), ImVec2(textPos.x + caretOffset, textPos.y + size),
                      color, 2.0f);
    drawList->PopClipRect();
}

} // namespace

void OnScreenKeyboard::Init() {
    ImGuiContextHook hook;
    hook.Type = ImGuiContextHookType_NewFramePre;
    hook.UserData = this;
    // Input editor windows unblock navigation every frame, so block it again right before ImGui reads the gamepad
    hook.Callback = [](ImGuiContext* context, ImGuiContextHook* newFrameHook) {
        if (static_cast<OnScreenKeyboard*>(newFrameHook->UserData)->IsTyping(*context)) {
            context->IO.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
        }
    };
    ImGui::AddContextHook(ImGui::GetCurrentContext(), &hook);
}

void OnScreenKeyboard::Update() {
#if defined(__ANDROID__) || defined(__IOS__)
    // These platforms open the system keyboard for every text field
    return;
#endif

    ImGuiContext& g = *GImGui;

    if (g.ActiveId != mGamepadField) {
        mGamepadField = 0;
    }

    // Text fields ignore A from a gamepad but open for a keyboard activation. Nothing else cares which it was.
    // A real press leaves out fields that focus on their own, such as a search box.
    if (g.NavActivatePressedId != 0 && g.NavInputSource == ImGuiInputSource_Gamepad) {
        g.NavInputSource = ImGuiInputSource_Keyboard;
        mGamepadField = g.NavActivatePressedId;
        mFieldJustOpened = true;
    }

    if (!IsTyping(g)) {
        return;
    }

    if (mFieldJustOpened) {
        mFieldJustOpened = false;
        ResetForNewField(g);
    }

    ImFont* font = ImGui::GetFont();
    BuildRows(font);
    HandleInput(g, font);
    BuildRows(font);

    // Fields read these as validate and cancel, including ones in windows drawn after navigation is unblocked
    for (ImGuiKey key : { ImGuiKey_NavGamepadActivate, ImGuiKey_NavGamepadCancel, ImGuiKey_NavGamepadInput }) {
        ImGui::SetKeyOwner(key, gKeyOwner, ImGuiInputFlags_LockThisFrame);
    }

    Draw(g, font);
}

void OnScreenKeyboard::SetAccentColor(const ImVec4& color) {
    mAccent = color;
}

unsigned int OnScreenKeyboard::Displayed(unsigned int c) const {
    return mMode == Mode::Letters && mShift != Shift::Off ? ToUpper(c) : c;
}

void OnScreenKeyboard::Type(unsigned int c) {
    ImGui::GetIO().AddInputCharacter(Displayed(c));
    if (mShift == Shift::Once) {
        mShift = Shift::Off;
    }
}

void OnScreenKeyboard::CycleShift() {
    mShift = mShift == Shift::Off ? Shift::Once : mShift == Shift::Once ? Shift::Locked : Shift::Off;
}

void OnScreenKeyboard::ChangeLanguage() {
    mLanguage = (mLanguage + 1) % gLanguageCount;
    auto context = Context::GetRawInstance();
    context->GetConsoleVariables()->SetInteger(CVAR_ONSCREEN_KEYBOARD_LANGUAGE, mLanguage);
    context->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

void OnScreenKeyboard::BuildRows(ImFont* font) {
    const Language& language = gLanguages[mLanguage];
    const char16_t* const* source =
        mMode == Mode::Letters ? language.Rows : gSymbolRows[mMode == Mode::MoreSymbols ? 1 : 0];

    std::vector<unsigned int> characters[3];
    mRowWidth = gMinRowWidth;
    for (int row = 0; row < 3; row++) {
        for (const char16_t* c = source[row]; *c != 0; c++) {
            const bool isCurrency = mMode == Mode::Symbols && *c == u'$';
            const unsigned int character = isCurrency ? language.Currency : *c;
            if (font->FindGlyphNoFallback(static_cast<ImWchar>(character)) != nullptr) {
                characters[row].push_back(character);
            }
        }
        mRowWidth = ImMax(mRowWidth, static_cast<float>(characters[row].size()));
    }

    for (int row = 0; row < 3; row++) {
        mRows[row].clear();
        const float side = (mRowWidth - characters[row].size()) / 2;
        if (row == 2) {
            mRows[row].push_back({ mMode == Mode::Letters ? KeyType::Shift : KeyType::SymbolPage, 0, 0.0f, side });
        }
        float x = side;
        for (unsigned int character : characters[row]) {
            mRows[row].push_back({ KeyType::Character, character, x, 1.0f });
            x += 1.0f;
        }
        if (row == 2) {
            mRows[row].push_back({ KeyType::Delete, 0, x, side });
        }
    }

    const float spaceWidth = mRowWidth - 6.0f;
    mRows[gKeyRows - 1] = {
        { KeyType::Mode, 0, 0.0f, 1.5f },
        { KeyType::Character, ',', 1.5f, 1.0f },
        { KeyType::Language, 0, 2.5f, 1.0f },
        { KeyType::Space, 0, 3.5f, spaceWidth },
        { KeyType::Character, '.', 3.5f + spaceWidth, 1.0f },
        { KeyType::Done, 0, 4.5f + spaceWidth, 1.5f },
    };

    mIndex = ImMin(mIndex, static_cast<int>(mRows[mRow].size()) - 1);
}

const OnScreenKeyboard::Key& OnScreenKeyboard::SelectedKey() const {
    return mRows[mRow][mIndex];
}

std::vector<unsigned int> OnScreenKeyboard::VariantsOf(const Key& key, int row, ImFont* font) const {
    std::vector<unsigned int> variants;
    if (key.Type != KeyType::Character) {
        return variants;
    }

    variants.push_back(key.Character);
    if (mMode == Mode::Letters && row == 0) {
        const int digit = static_cast<int>(&key - mRows[0].data());
        if (digit < 10) {
            variants.push_back('0' + (digit + 1) % 10);
        }
    }
    for (const Variants& entry : gVariants) {
        if (entry.Base != key.Character) {
            continue;
        }
        for (const char16_t* c = entry.Characters; *c != 0; c++) {
            if (font->FindGlyphNoFallback(static_cast<ImWchar>(Displayed(*c))) != nullptr) {
                variants.push_back(*c);
            }
        }
    }
    return variants;
}

void OnScreenKeyboard::MoveVertically(int direction) {
    const Key& current = SelectedKey();
    const float center = current.X + current.Width / 2;
    mRow = (mRow + direction + gKeyRows) % gKeyRows;

    float closest = FLT_MAX;
    for (int i = 0; i < static_cast<int>(mRows[mRow].size()); i++) {
        const Key& key = mRows[mRow][i];
        const float distance = ImAbs(key.X + key.Width / 2 - center);
        if (distance < closest) {
            closest = distance;
            mIndex = i;
        }
    }
}

void OnScreenKeyboard::PressSelectedKey(ImGuiContext& g) {
    const Key& key = SelectedKey();
    switch (key.Type) {
        case KeyType::Character:
            Type(key.Character);
            break;
        case KeyType::Shift:
            CycleShift();
            break;
        case KeyType::SymbolPage:
            mMode = mMode == Mode::Symbols ? Mode::MoreSymbols : Mode::Symbols;
            break;
        case KeyType::Mode:
            mMode = mMode == Mode::Letters ? Mode::Symbols : Mode::Letters;
            break;
        case KeyType::Delete:
            TapKey(ImGuiKey_Backspace);
            break;
        case KeyType::Language:
            ChangeLanguage();
            break;
        case KeyType::Space:
            g.IO.AddInputCharacter(' ');
            break;
        case KeyType::Done:
            Finish(g);
            break;
    }
}

void OnScreenKeyboard::HandleInput(ImGuiContext& g, ImFont* font) {
    const bool left = AnyPressed(mBindings.Left.Keys) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft);
    const bool right = AnyPressed(mBindings.Right.Keys) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight);
    const float typeHeld = HeldSeconds(mBindings.Type.Keys);

    if (AnyPressed(mBindings.Type.Keys, false)) {
        mConfirmHeld = true;
    }

    if (mVariantsOpen) {
        const std::vector<unsigned int> variants = VariantsOf(SelectedKey(), mRow, font);
        const int count = static_cast<int>(variants.size());
        mVariantIndex = (mVariantIndex + (right ? 1 : 0) - (left ? 1 : 0) + count) % count;
        if (typeHeld < 0.0f) {
            Type(variants[mVariantIndex]);
            mVariantsOpen = false;
            mConfirmHeld = false;
        }
        return;
    }

    if (mConfirmHeld) {
        if (typeHeld < 0.0f) {
            mConfirmHeld = false;
            PressSelectedKey(g);
            return;
        }
        if (typeHeld >= gHoldSeconds && VariantsOf(SelectedKey(), mRow, font).size() > 1) {
            mVariantsOpen = true;
            mVariantIndex = 0;
            return;
        }
    }

    const int rowSize = static_cast<int>(mRows[mRow].size());
    if (left) {
        mIndex = (mIndex + rowSize - 1) % rowSize;
    }
    if (right) {
        mIndex = (mIndex + 1) % rowSize;
    }
    if (AnyPressed(mBindings.Up.Keys) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp)) {
        MoveVertically(-1);
    }
    if (AnyPressed(mBindings.Down.Keys) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown)) {
        MoveVertically(1);
    }

    if (AnyPressed(mBindings.Space.Keys, false)) {
        g.IO.AddInputCharacter(' ');
    }
    if (AnyPressed(mBindings.Shift.Keys, false) && mMode == Mode::Letters) {
        CycleShift();
    }
    if (AnyPressed(mBindings.CursorLeft.Keys)) {
        TapKey(ImGuiKey_LeftArrow);
    }
    if (AnyPressed(mBindings.CursorRight.Keys)) {
        TapKey(ImGuiKey_RightArrow);
    }
    if (AnyPressed(mBindings.Language.Keys, false)) {
        ChangeLanguage();
    }

    if (g.InputTextState.TextLen > 0 && AnyPressed(mBindings.Remove.Keys)) {
        TapKey(ImGuiKey_Backspace);
    } else if (AnyPressed(mBindings.Remove.Keys, false)) {
        Finish(g);
    }
    if (AnyPressed(mBindings.Done.Keys, false)) {
        Finish(g);
    }
}

std::string OnScreenKeyboard::KeyLabel(const Key& key) const {
    switch (key.Type) {
        case KeyType::Character:
            return ToUtf8(Displayed(key.Character));
        case KeyType::Shift:
            return ICON_FA_ARROW_UP;
        case KeyType::SymbolPage:
            return mMode == Mode::Symbols ? "=\\<" : "?123";
        case KeyType::Mode:
            return mMode == Mode::Letters ? "?123" : "ABC";
        case KeyType::Delete:
            return ICON_FA_LONG_ARROW_LEFT;
        case KeyType::Language:
            return gLanguages[mLanguage].Code;
        case KeyType::Space:
            return gLanguages[mLanguage].Name;
        case KeyType::Done:
            return ICON_FA_CHECK;
    }
    return "";
}

void OnScreenKeyboard::Draw(ImGuiContext& g, ImFont* font) {
    ImGuiViewport* viewport = g.ActiveIdWindow != nullptr ? g.ActiveIdWindow->Viewport : ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetForegroundDrawList(viewport);
    const ImVec2 display = viewport->Size;

    // Follows the font scale, but always fits on screen
    const float scaledKey = display.y / 11 * g.IO.FontGlobalScale / 1.5f;
    const float keySize = ImMin(scaledKey, ImMin(display.x / (mRowWidth + 1), display.y / (gKeyRows + 3)));
    const float keyFontSize = keySize / 2;
    const float smallFontSize = keySize / 4;
    const float hintFontSize = ImMax(keySize / 3.5f, ImGui::GetFontSize());
    const float padding = keySize / 4;
    const float gap = keySize / 16;
    const float hintWrap = display.x - padding * 4;
    const ImVec2 hintSize = font->CalcTextSizeA(hintFontSize, FLT_MAX, hintWrap, mHint.c_str());
    const float keysWidth = keySize * mRowWidth;
    const ImVec2 panelSize(ImMax(keysWidth, hintSize.x) + padding * 2,
                           keySize * (gKeyRows + 1) + hintSize.y + padding * 4);

    const ImU32 keyColor = ImGui::ColorConvertFloat4ToU32(ImVec4(mAccent.x, mAccent.y, mAccent.z, 0.35f));
    const ImU32 actionColor = ImGui::ColorConvertFloat4ToU32(ImVec4(mAccent.x, mAccent.y, mAccent.z, 0.15f));
    const ImU32 fullColor = ImGui::ColorConvertFloat4ToU32(ImVec4(mAccent.x, mAccent.y, mAccent.z, 1.0f));
    const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
    const bool isBrightTheme = mAccent.x * 0.3f + mAccent.y * 0.6f + mAccent.z * 0.1f > 0.5f;
    const ImU32 textOnFull = isBrightTheme ? IM_COL32_BLACK : textColor;

    const bool fieldInLowerHalf = g.PlatformImeData.InputPos.y > viewport->Pos.y + display.y / 2;
    const ImVec2 panelMin(viewport->Pos.x + (display.x - panelSize.x) / 2,
                          viewport->Pos.y + (fieldInLowerHalf ? padding : display.y - panelSize.y - padding));
    drawList->AddRectFilled(panelMin, panelMin + panelSize, IM_COL32(0, 0, 0, 235), padding);

    const ImVec2 keysMin(panelMin.x + (panelSize.x - keysWidth) / 2, panelMin.y + keySize + padding * 2);
    const ImVec2 previewMin(keysMin.x, panelMin.y + padding);
    const ImVec2 previewMax(keysMin.x + keysWidth - gap, previewMin.y + keySize);
    drawList->AddRectFilled(previewMin, previewMax, actionColor, padding / 2);
    DrawPreview(g, drawList, font, keyFontSize, previewMin, previewMax, textColor);

    ImVec2 selectedMin;
    for (int row = 0; row < gKeyRows; row++) {
        for (int i = 0; i < static_cast<int>(mRows[row].size()); i++) {
            const Key& key = mRows[row][i];
            const ImVec2 min = keysMin + ImVec2(key.X * keySize, row * keySize);
            const ImVec2 max = min + ImVec2(key.Width * keySize, keySize) - ImVec2(gap, gap);
            const bool selected = row == mRow && i == mIndex;
            const bool lit = key.Type == KeyType::Done || (key.Type == KeyType::Shift && mShift != Shift::Off);
            const bool filled = selected || lit;
            drawList->AddRectFilled(min, max,
                                    filled                           ? fullColor
                                    : key.Type == KeyType::Character ? keyColor
                                                                     : actionColor,
                                    padding / 2);
            if (selected) {
                selectedMin = min;
                drawList->AddRect(min, max, IM_COL32_WHITE, padding / 2, 0, 2.0f);
            }
            if (key.Type == KeyType::Shift && mShift == Shift::Locked) {
                drawList->AddLine(ImVec2(min.x + padding, max.y - gap * 2), ImVec2(max.x - padding, max.y - gap * 2),
                                  textOnFull, 2.0f);
            }

            const float labelSize = key.Type == KeyType::Space ? keySize / 3 : keyFontSize;
            DrawCentered(drawList, font, labelSize, min, max, filled ? textOnFull : textColor, KeyLabel(key));

            const std::vector<unsigned int> variants = VariantsOf(key, row, font);
            if (variants.size() > 1) {
                const std::string hint = ToUtf8(Displayed(variants[1]));
                const ImVec2 hintMin(max.x - smallFontSize, min.y + gap);
                drawList->AddText(font, smallFontSize, hintMin, filled ? textOnFull : textColor, hint.c_str());
            }
        }
    }

    const ImVec2 hintPos(panelMin.x + (panelSize.x - hintSize.x) / 2, keysMin.y + keySize * gKeyRows + padding);
    drawList->AddText(font, hintFontSize, hintPos, textColor, mHint.c_str(), nullptr, hintWrap);

    if (!mVariantsOpen) {
        return;
    }

    const std::vector<unsigned int> variants = VariantsOf(SelectedKey(), mRow, font);
    const float stripWidth = keySize * variants.size();
    const float stripX = ImClamp(selectedMin.x, panelMin.x, panelMin.x + panelSize.x - stripWidth);
    const float stripY = ImMax(selectedMin.y - keySize, panelMin.y);
    const ImVec2 stripMin(stripX, stripY);
    drawList->AddRectFilled(stripMin, stripMin + ImVec2(stripWidth, keySize), IM_COL32(0, 0, 0, 255), padding / 2);
    drawList->AddRect(stripMin, stripMin + ImVec2(stripWidth, keySize), IM_COL32_WHITE, padding / 2, 0, 2.0f);
    for (int i = 0; i < static_cast<int>(variants.size()); i++) {
        const ImVec2 min = stripMin + ImVec2(i * keySize, 0.0f);
        const ImVec2 max = min + ImVec2(keySize, keySize);
        const bool selected = i == mVariantIndex;
        if (selected) {
            drawList->AddRectFilled(min, max, fullColor, padding / 2);
        }
        DrawCentered(drawList, font, keyFontSize, min, max, selected ? textOnFull : textColor,
                     ToUtf8(Displayed(variants[i])));
    }
}

bool OnScreenKeyboard::IsTyping(const ImGuiContext& g) const {
    return mGamepadField != 0 && g.ActiveId == mGamepadField && g.InputTextState.ID == mGamepadField &&
           !(g.InputTextState.Flags & ImGuiInputTextFlags_ReadOnly);
}

void OnScreenKeyboard::BuildHint() {
    mHint.clear();
    const auto add = [this](const std::string& buttons, const char* action) {
        if (!buttons.empty()) {
            mHint += (mHint.empty() ? "" : "   ") + buttons + " " + action;
        }
    };
    const std::string& cursorLeft = mBindings.CursorLeft.Label;
    const std::string& cursorRight = mBindings.CursorRight.Label;
    add(mBindings.Type.Label, "Type (Hold: Accents)");
    add(mBindings.Remove.Label, "Delete");
    add(mBindings.Shift.Label, "Shift");
    add(mBindings.Space.Label, "Space");
    add(cursorLeft + (cursorLeft.empty() || cursorRight.empty() ? "" : "/") + cursorRight, "Cursor");
    add(mBindings.Language.Label, "Language");
    add(mBindings.Done.Label, "Done");
}

void OnScreenKeyboard::ResetForNewField(const ImGuiContext& g) {
    mMode = (g.InputTextState.Flags & gNumericFlags) ? Mode::Symbols : Mode::Letters;
    mShift = Shift::Off;
    mRow = 0;
    mIndex = 0;
    mConfirmHeld = false;
    mVariantsOpen = false;

    auto context = Context::GetRawInstance();
    const int language = context->GetConsoleVariables()->GetInteger(CVAR_ONSCREEN_KEYBOARD_LANGUAGE, 0);
    mLanguage = ImClamp(language, 0, gLanguageCount - 1);

    mBindings = context->GetControlDeck()->GetOnScreenKeyboardBindings();

    const OnScreenKeyboardBindings defaults;
    const auto require = [](OnScreenKeyboardBinding& binding, const OnScreenKeyboardBinding& fallback) {
        if (binding.Keys.empty()) {
            binding = fallback;
        }
    };
    require(mBindings.Type, defaults.Type);
    require(mBindings.Up, defaults.Up);
    require(mBindings.Down, defaults.Down);
    require(mBindings.Left, defaults.Left);
    require(mBindings.Right, defaults.Right);
    BuildHint();
    TapKey(ImGuiKey_End);
}
} // namespace Ship
