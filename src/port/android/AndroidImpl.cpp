#ifdef __ANDROID__
#include "AndroidImpl.h"
#include <SDL2/SDL.h>
#include "public/bridge/consolevariablebridge.h"

#include <ImGui/imgui_internal.h>

static bool isShowingVirtualKeyboard = true;

void ShipDK::Android::ImGuiProcessEvent(bool wantsTextInput) {
    ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetActiveID());

    if (wantsTextInput) {
        if (!isShowingVirtualKeyboard) {
            state->ClearText();

            isShowingVirtualKeyboard = true;
            SDL_StartTextInput();
        }
    } else {
        if (isShowingVirtualKeyboard) {
            isShowingVirtualKeyboard = false;
            SDL_StopTextInput();
        }
    }
}
#endif
