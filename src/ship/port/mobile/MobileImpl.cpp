#if defined(__ANDROID__) || defined(__IOS__)
#include "ship/port/mobile/MobileImpl.h"
#include <SDL3/SDL.h>

#include <imgui_internal.h>

static bool isShowingVirtualKeyboard = true;

void Ship::Mobile::ImGuiProcessEvent(SDL_Window* window, bool wantsTextInput) {
    ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetActiveID());

    if (wantsTextInput) {
        if (!isShowingVirtualKeyboard) {
            state->ClearText();

            isShowingVirtualKeyboard = true;
            SDL_StartTextInput(window);
        }
    } else {
        if (isShowingVirtualKeyboard) {
            isShowingVirtualKeyboard = false;
            SDL_StopTextInput(window);
        }
    }
}
#endif
