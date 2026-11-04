#pragma once

#include <imgui.h>
#include <string>
#include <vector>

namespace Ship {

/** @brief The gamepad keys behind an OnScreenKeyboard action. */
struct OnScreenKeyboardBinding {
    std::vector<ImGuiKey> Keys; ///< Any of these triggers the action.
    std::string Label = "";     ///< Names the keys in the hint under the keyboard.
};

/**
 * @brief The bindings behind each OnScreenKeyboard action.
 *
 * An action with no keys is disabled and left out of the hint, except Type and the directions, which fall back
 * to these defaults.
 */
struct OnScreenKeyboardBindings {
    OnScreenKeyboardBinding Type = { { ImGuiKey_GamepadFaceDown }, "A" };    ///< Press a key, or hold for accents.
    OnScreenKeyboardBinding Remove = { { ImGuiKey_GamepadFaceRight }, "B" }; ///< Delete, or leave an empty field.
    OnScreenKeyboardBinding Shift = { { ImGuiKey_GamepadFaceLeft }, "X" };   ///< Shift once, lock, then off.
    OnScreenKeyboardBinding Space = { { ImGuiKey_GamepadFaceUp }, "Y" };     ///< Type a space.
    OnScreenKeyboardBinding CursorLeft = { { ImGuiKey_GamepadL1 }, "LB" };   ///< Move the text cursor left.
    OnScreenKeyboardBinding CursorRight = { { ImGuiKey_GamepadR1 }, "RB" };  ///< Move the text cursor right.
    OnScreenKeyboardBinding Language = { { ImGuiKey_GamepadL2, ImGuiKey_GamepadR2 }, "LT/RT" }; ///< Next layout.
    OnScreenKeyboardBinding Done = { { ImGuiKey_GamepadStart }, "Start" };                      ///< Leave the field.
    OnScreenKeyboardBinding Up = { { ImGuiKey_GamepadDpadUp } };       ///< Select the key above.
    OnScreenKeyboardBinding Down = { { ImGuiKey_GamepadDpadDown } };   ///< Select the key below.
    OnScreenKeyboardBinding Left = { { ImGuiKey_GamepadDpadLeft } };   ///< Select the key to the left.
    OnScreenKeyboardBinding Right = { { ImGuiKey_GamepadDpadRight } }; ///< Select the key to the right.
};

/**
 * @brief Types into ImGui text fields with a gamepad.
 *
 * Opens over any text field activated with gamepad navigation. Keys the current font can't draw are left out,
 * so the layouts follow the glyph ranges a port loads. ImGui only loads Latin-1 by default.
 *
 * Android and iOS keep their system keyboard.
 *
 * Obtain the instance from Gui::GetOnScreenKeyboard().
 */
class OnScreenKeyboard {
  public:
    void Init();
    void Update();
    void SetAccentColor(const ImVec4& color);

  private:
    static constexpr int gKeyRows = 4;

    enum class KeyType { Character, Shift, SymbolPage, Mode, Delete, Language, Space, Done };
    enum class Mode { Letters, Symbols, MoreSymbols };
    enum class Shift { Off, Once, Locked };

    struct Key {
        KeyType Type;
        unsigned int Character;
        float X;
        float Width;
    };

    unsigned int Displayed(unsigned int c) const;
    void Type(unsigned int c);
    void CycleShift();
    void ChangeLanguage();
    void BuildRows(ImFont* font);
    const Key& SelectedKey() const;
    std::vector<unsigned int> VariantsOf(const Key& key, int row, ImFont* font) const;
    void MoveVertically(int direction);
    void PressSelectedKey(ImGuiContext& g);
    void HandleInput(ImGuiContext& g, ImFont* font);
    std::string KeyLabel(const Key& key) const;
    void Draw(ImGuiContext& g, ImFont* font);
    bool IsTyping(const ImGuiContext& g) const;
    void BuildHint();
    void ResetForNewField(const ImGuiContext& g);

    std::vector<Key> mRows[gKeyRows];
    float mRowWidth = 0.0f;
    int mRow = 0;
    int mIndex = 0;
    int mLanguage = 0;
    Mode mMode = Mode::Letters;
    Shift mShift = Shift::Off;
    OnScreenKeyboardBindings mBindings;
    std::string mHint;
    ImVec4 mAccent = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
    bool mConfirmHeld = false;
    bool mVariantsOpen = false;
    int mVariantIndex = 0;
    ImGuiID mGamepadField = 0;
    bool mFieldJustOpened = false;
};
} // namespace Ship
