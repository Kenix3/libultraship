#include "ship/controller/physicaldevice/SDLAddRemoveDeviceEventHandler.h"
#include <SDL2/SDL.h>
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
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEADDED, SDL_CONTROLLERDEVICEADDED) > 0) {
        // from https://wiki.libsdl.org/SDL2/SDL_ControllerDeviceEvent: which - the joystick device index for
        // the SDL_CONTROLLERDEVICEADDED event
        if (mControlDeck) {
            mControlDeck->GetConnectedPhysicalDeviceManager()->HandlePhysicalDeviceConnect(event.cdevice.which);
            changed = true;
        }
    }

    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEREMOVED, SDL_CONTROLLERDEVICEREMOVED) > 0) {
        // from https://wiki.libsdl.org/SDL2/SDL_ControllerDeviceEvent: which - the [...] instance id for the
        // SDL_CONTROLLERDEVICEREMOVED [...] event
        if (mControlDeck) {
            mControlDeck->GetConnectedPhysicalDeviceManager()->HandlePhysicalDeviceDisconnect(event.cdevice.which);
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
