#include "ship/controller/physicaldevice/SDLAddRemoveDeviceEventHandler.h"
#include <SDL3/SDL.h>
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

namespace Ship {

SDLAddRemoveDeviceEventHandler::SDLAddRemoveDeviceEventHandler(std::shared_ptr<ConsoleVariable> consoleVariable,
                                                               std::shared_ptr<Window> window,
                                                               std::shared_ptr<ControlDeck> controlDeck,
                                                               const std::string& visibilityCvar,
                                                               const std::string& name)
    : GuiWindow(std::move(consoleVariable), std::move(window), visibilityCvar, false, name, ImVec2{ -1, -1 },
                ImGuiWindowFlags_None),
      mControlDeck(std::move(controlDeck)) {
}

SDLAddRemoveDeviceEventHandler::~SDLAddRemoveDeviceEventHandler() {
}

void SDLAddRemoveDeviceEventHandler::DrawElement() {
}

void SDLAddRemoveDeviceEventHandler::UpdateElement() {
    SDL_PumpEvents();
    SDL_Event event;
    bool changed = false;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_GAMEPAD_ADDED, SDL_EVENT_GAMEPAD_ADDED) > 0) {
        if (mControlDeck) {
            mControlDeck->GetConnectedPhysicalDeviceManager()->HandlePhysicalDeviceConnect(event.gdevice.which);
            changed = true;
        }
    }

    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_GAMEPAD_REMOVED, SDL_EVENT_GAMEPAD_REMOVED) > 0) {
        if (mControlDeck) {
            mControlDeck->GetConnectedPhysicalDeviceManager()->HandlePhysicalDeviceDisconnect(event.gdevice.which);
            changed = true;
        }
    }

    // The connected controller set changed, so re-point the ImGui gamepad
    // backend at it (keeps menu navigation working across hotplug).
    if (changed) {
        auto window = GetWindow();
        if (window != nullptr && window->GetGui() != nullptr) {
            window->GetGui()->RefreshImGuiGamepads();
        }
    }
}
} // namespace Ship
