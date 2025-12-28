#include "ship/controller/controldeck/ControlDeck.h"

#include "ship/Context.h"
#include "ship/controller/controldevice/controller/Controller.h"
#include "ship/utils/StringHelper.h"
#include "ship/config/ConsoleVariable.h"
#include <imgui.h>
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"

namespace Ship {

ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                         std::shared_ptr<ControllerDefaultMappings> controllerDefaultMappings,
                         std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames) {
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
    return AllGameInputBlocked() ||
           Context::GetInstance()->GetWindow()->GetGui()->GetMenuOrMenubarVisible() &&
               Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_IMGUI_CONTROLLER_NAV, 0);
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

const std::unordered_map<CONTROLLERBUTTONS_T, std::string>& ControlDeck::GetAllButtonNames() const {
    return mButtonNames;
}

std::string ControlDeck::GetButtonNameForBitmask(CONTROLLERBUTTONS_T bitmask) {
    // if we don't have a name for this bitmask,
    // return the stringified bitmask
    if (!mButtonNames.contains(bitmask)) {
        return std::to_string(bitmask);
    }

    return mButtonNames[bitmask];
}
} // namespace Ship
