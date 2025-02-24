#include "ControlDeck.h"

#include "Context.h"
#include "controller/controldevice/controller/Controller.h"
#include "utils/StringHelper.h"
#include "public/bridge/consolevariablebridge.h"
#include <imgui.h>
#include "controller/controldevice/controller/mapping/mouse/WheelHandler.h"

namespace Ship {

ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                         std::shared_ptr<ControllerDefaultMappings> controllerDefaultMappings) {
    mConnectedPhysicalDeviceManager = std::make_shared<ConnectedPhysicalDeviceManager>();
    mGlobalSDLDeviceSettings = std::make_shared<GlobalSDLDeviceSettings>();
    mControllerDefaultMappings = controllerDefaultMappings == nullptr ? std::make_shared<ControllerDefaultMappings>()
                                                                      : controllerDefaultMappings;
}

ControlDeck::~ControlDeck() {
    SPDLOG_TRACE("destruct control deck");
}

void ControlDeck::Init(uint8_t* controllerBits) {
    mControllerBits = controllerBits;
    *mControllerBits |= 1 << 0;

    for (auto port : mPorts) {
        if (port->GetConnectedController()->HasConfig()) {
            port->GetConnectedController()->ReloadAllMappingsFromConfig();
        }
    }

    // if we don't have a config for controller 1, set default bindings
    if (!mPorts[0]->GetConnectedController()->HasConfig()) {
        mPorts[0]->GetConnectedController()->AddDefaultMappings(PhysicalDeviceType::Keyboard);
        mPorts[0]->GetConnectedController()->AddDefaultMappings(PhysicalDeviceType::Mouse);
        mPorts[0]->GetConnectedController()->AddDefaultMappings(PhysicalDeviceType::SDLGamepad);
    }
}

bool ControlDeck::ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode) {
    bool result = false;
    for (auto port : mPorts) {
        auto controller = port->GetConnectedController();

        if (controller != nullptr) {
            result = controller->ProcessKeyboardEvent(eventType, scancode) || result;
        }
    }

    return result;
}

bool ControlDeck::ProcessMouseButtonEvent(bool isPressed, MouseBtn button) {
    bool result = false;
    for (auto port : mPorts) {
        auto controller = port->GetConnectedController();

        if (controller != nullptr) {
            result = controller->ProcessMouseButtonEvent(isPressed, button) || result;
        }
    }

    return result;
}

bool ControlDeck::AllGameInputBlocked() {
    return !mGameInputBlockers.empty();
}

bool ControlDeck::GamepadGameInputBlocked() {
    // block controller input when using the controller to navigate imgui menus
    return AllGameInputBlocked() || Context::GetInstance()->GetWindow()->GetGui()->GetMenuOrMenubarVisible() &&
                                        CVarGetInteger(CVAR_IMGUI_CONTROLLER_NAV, 0);
}

bool ControlDeck::KeyboardGameInputBlocked() {
    // block keyboard input when typing in imgui
    ImGuiWindow* activeIDWindow = ImGui::GetCurrentContext()->ActiveIdWindow;
    return AllGameInputBlocked() ||
           (activeIDWindow != NULL &&
            activeIDWindow->ID != Context::GetInstance()->GetWindow()->GetGui()->GetMainGameWindowID()) ||
           ImGui::GetTopMostPopupModal() != NULL; // ImGui::GetIO().WantCaptureKeyboard, but ActiveId check altered
}

bool ControlDeck::MouseGameInputBlocked() {
    // block mouse input when user interacting with gui
    ImGuiWindow* window = ImGui::GetCurrentContext()->HoveredWindow;
    if (window == NULL) {
        return true;
    }
    return AllGameInputBlocked() ||
           (window->ID != Context::GetInstance()->GetWindow()->GetGui()->GetMainGameWindowID());
}

std::shared_ptr<Controller> ControlDeck::GetControllerByPort(uint8_t port) {
    return mPorts[port]->GetConnectedController();
}

void ControlDeck::BlockGameInput(int32_t blockId) {
    mGameInputBlockers[blockId] = true;
}

void ControlDeck::UnblockGameInput(int32_t blockId) {
    mGameInputBlockers.erase(blockId);
}

std::shared_ptr<ConnectedPhysicalDeviceManager> ControlDeck::GetConnectedPhysicalDeviceManager() {
    return mConnectedPhysicalDeviceManager;
}

std::shared_ptr<GlobalSDLDeviceSettings> ControlDeck::GetGlobalSDLDeviceSettings() {
    return mGlobalSDLDeviceSettings;
}

std::shared_ptr<ControllerDefaultMappings> ControlDeck::GetControllerDefaultMappings() {
    return mControllerDefaultMappings;
}
} // namespace Ship

namespace LUS {
ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                         std::shared_ptr<Ship::ControllerDefaultMappings> controllerDefaultMappings)
    : Ship::ControlDeck(additionalBitmasks, controllerDefaultMappings), mPads(nullptr) {
    for (int32_t i = 0; i < MAXCONTROLLERS; i++) {
        mPorts.push_back(std::make_shared<Ship::ControlPort>(i, std::make_shared<Controller>(i, additionalBitmasks)));
    }
}

ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks)
    : ControlDeck(additionalBitmasks, nullptr) {
}

ControlDeck::ControlDeck() : ControlDeck(std::vector<CONTROLLERBUTTONS_T>()) {
}

OSContPad* ControlDeck::GetPads() {
    return mPads;
}

void ControlDeck::WriteToPad(void* pad) {
    WriteToOSContPad((OSContPad*)pad);
}

void ControlDeck::WriteToOSContPad(OSContPad* pad) {
    SDL_PumpEvents();
    Ship::WheelHandler::GetInstance()->Update();

    if (AllGameInputBlocked()) {
        return;
    }

    mPads = pad;

    for (size_t i = 0; i < mPorts.size(); i++) {
        const std::shared_ptr<Ship::Controller> controller = mPorts[i]->GetConnectedController();

        if (controller != nullptr) {
            controller->ReadToPad(&pad[i]);
        }
    }
}
} // namespace LUS
