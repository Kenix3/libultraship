#include "ControlDeck.h"

#include "Context.h"
#include "controller/controldevice/controller/Controller.h"
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"
#include <imgui.h>

// #ifdef __WIIU__
// #include "port/wiiu/WiiUGamepad.h"
// #include "port/wiiu/WiiUController.h"
// #endif

namespace LUS {

ControlDeck::ControlDeck() : mPads(nullptr) {
    for (int32_t i = 0; i < 4; i++) {
        mPorts.push_back(std::make_shared<ControlPort>(i, std::make_shared<Controller>(i)));
    }
    mGameInputBlocked = false;
}

ControlDeck::~ControlDeck() {
    SPDLOG_TRACE("destruct control deck");
}

void ControlDeck::Init(uint8_t* bits) {
    mControllerBits = bits;
    *mControllerBits |= 1 << 0;

    for (auto port : mPorts) {
        if (port->GetConnectedController()->HasConfig()) {
            port->GetConnectedController()->ReloadAllMappingsFromConfig();
        } else {
            port->GetConnectedController()->ResetToDefaultMappings(port->GetConnectedController()->GetPort());
        }
    }
}

// void ControlDeck::ScanDevices() {
//     mPortList.clear();
//     mDevices.clear();

//     // Always load controllers that need their device indices zero based first because we add some other devices
//     // afterward.
//     int32_t i;

// #ifndef __WIIU__
//     for (i = 0; i < SDL_NumJoysticks(); i++) {
//         if (SDL_IsGameController(i)) {
//             auto sdl = std::make_shared<SDLController>(i);
//             sdl->Open();
//             mDevices.push_back(sdl);
//         }
//     }

//     mDevices.push_back(std::make_shared<DummyController>(i++, "Auto", "Auto", true));
//     mDevices.push_back(std::make_shared<KeyboardController>(i++));
// #else
//     for (i = 0; i < 4; i++) {
//         auto controller = std::make_shared<LUS::WiiUController>(i, (WPADChan)i);
//         controller->Open();
//         mDevices.push_back(controller);
//     }

//     auto gamepad = std::make_shared<LUS::WiiUGamepad>(i++);
//     gamepad->Open();
//     mDevices.push_back(gamepad);

//     mDevices.push_back(std::make_shared<DummyController>(i++, "Auto", "Auto", true));
// #endif

//     mDevices.push_back(std::make_shared<DummyController>(i++, "Disconnected", "None", false));

//     for (const auto& device : mDevices) {
//         for (int32_t i = 0; i < MAXCONTROLLERS; i++) {
//             device->CreateDefaultBinding(i);
//         }
//     }

//     for (int32_t i = 0; i < MAXCONTROLLERS; i++) {
//         mPortList.push_back(i == 0 ? 0 : static_cast<int>(mDevices.size()) - 1);
//     }

//     LoadSettings();
// }

void ControlDeck::WriteToPad(OSContPad* pad) {
    if (mGameInputBlocked) {
        return;
    }

    mPads = pad;

    for (size_t i = 0; i < mPorts.size(); i++) {
        const std::shared_ptr<Controller> controller = mPorts[i]->GetConnectedController(); 

        if (controller != nullptr) {
            controller->ReadToPad(&pad[i]);
        }
    }
}

OSContPad* ControlDeck::GetPads() {
    return mPads;
}

std::shared_ptr<Controller> ControlDeck::GetControllerByPort(uint8_t port) {
    return mPorts[port]->GetConnectedController();
}

// size_t ControlDeck::GetNumDevices() {
//     return mDevices.size();
// }

// int32_t ControlDeck::GetDeviceIndexFromPortIndex(int32_t portIndex) {
//     return mPortList[portIndex];
// }

// size_t ControlDeck::GetNumConnectedPorts() {
//     return mPortList.size();
// }

// std::shared_ptr<Controller> ControlDeck::GetDeviceFromPortIndex(int32_t portIndex) {
//     return GetDeviceFromDeviceIndex(GetDeviceIndexFromPortIndex(portIndex));
// }

// uint8_t* ControlDeck::GetControllerBits() {
//     return mControllerBits;
// }

void ControlDeck::BlockGameInput() {
    mGameInputBlocked = true;
}

void ControlDeck::UnblockGameInput() {
    mGameInputBlocked = false;
}

// bool ControlDeck::IsBlockingGameInput(const std::string& inputDeviceGuid) const {
//     // We block controller input if F1 menu is open and control navigation is on.
//     // This is because we don't want controller inputs to affect the game
//     bool shouldBlockControllerInput = CVarGetInteger("gOpenMenuBar", 0) && CVarGetInteger("gControlNav", 0);

//     // We block keyboard input if you're currently typing into a textfield.
//     // This is because we don't want your keyboard typing to affect the game.
//     ImGuiIO io = ImGui::GetIO();
//     bool shouldBlockKeyboardInput = io.WantCaptureKeyboard;

//     bool inputDeviceIsKeyboard = inputDeviceGuid == "Keyboard";
//     return (!mGameInputBlockers.empty()) ||
//            (inputDeviceIsKeyboard ? shouldBlockKeyboardInput : shouldBlockControllerInput);
// }
} // namespace LUS
