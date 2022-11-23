#include "ControlDeck.h"

#include "core/Window.h"
#include "Controller.h"
#include "DummyController.h"
#include <Utils/StringHelper.h>
#include "core/bridge/consolevariablebridge.h"
#include <imgui.h>

#ifndef __WIIU__
#include "controller/KeyboardController.h"
#include "controller/SDLController.h"
#else
#include "port/wiiu/WiiUGamepad.h"
#include "port/wiiu/WiiUController.h"
#endif

namespace Ship {

void ControlDeck::Init(uint8_t* bits) {
    ScanPhysicalDevices();
    mControllerBits = bits;
}

void ControlDeck::ScanPhysicalDevices() {

    mVirtualDevices.clear();
    mPhysicalDevices.clear();

#ifndef __WIIU__
    for (int32_t i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            auto sdl = std::make_shared<SDLController>(i);
            sdl->Open();
            mPhysicalDevices.push_back(sdl);
        }
    }

    mPhysicalDevices.push_back(std::make_shared<DummyController>("Auto", "Auto", true));
    mPhysicalDevices.push_back(std::make_shared<KeyboardController>());
#else
    mPhysicalDevices.push_back(std::make_shared<DummyController>("Auto", "Auto", true));

    auto gamepad = std::make_shared<Ship::WiiUGamepad>();
    gamepad->Open();
    mPhysicalDevices.push_back(gamepad);

    for (int32_t i = 0; i < 4; i++) {
        auto controller = std::make_shared<Ship::WiiUController>((WPADChan)i);
        controller->Open();
        mPhysicalDevices.push_back(controller);
    }
#endif

    mPhysicalDevices.push_back(std::make_shared<DummyController>("Disconnected", "None", false));

    for (const auto device : mPhysicalDevices) {
        for (int32_t i = 0; i < MAXCONTROLLERS; i++) {
            device->CreateDefaultBinding(i);
        }
    }

    for (int32_t i = 0; i < MAXCONTROLLERS; i++) {
        mVirtualDevices.push_back(i == 0 ? 0 : static_cast<int>(mPhysicalDevices.size()) - 1);
    }

    LoadControllerSettings();
}

void ControlDeck::SetPhysicalDevice(int32_t slot, int32_t deviceSlot) {
    const std::shared_ptr<Controller> backend = mPhysicalDevices[deviceSlot];
    mVirtualDevices[slot] = deviceSlot;
    *mControllerBits |= (backend->Connected()) << slot;
}

void ControlDeck::WriteToPad(OSContPad* pad) const {
    for (size_t i = 0; i < mVirtualDevices.size(); i++) {
        const std::shared_ptr<Controller> backend = mPhysicalDevices[mVirtualDevices[i]];

        // If the controller backend is "Auto" we need to get the real device
        // we search for the real device to read input from it
        if (backend->GetGuid() == "Auto") {
            for (const auto& device : mPhysicalDevices) {
                if (ShouldBlockGameInput(device->GetGuid())) {
                    device->Read(nullptr, i);
                    continue;
                }

                device->Read(&pad[i], i);
            }
            continue;
        }

        if (ShouldBlockGameInput(backend->GetGuid())) {
            backend->Read(nullptr, i);
            continue;
        }

        backend->Read(&pad[i], i);
    }
}

#define NESTED(key, ...) \
    StringHelper::Sprintf("Controllers.%s.Slot_%d." key, device->GetGuid().c_str(), virtualSlot, __VA_ARGS__)

void ControlDeck::LoadControllerSettings() {
    std::shared_ptr<Mercury> config = Window::GetInstance()->GetConfig();

    for (auto const& val : config->rjson["Controllers"]["Deck"].items()) {
        int32_t slot = std::stoi(val.key().substr(5));

        for (size_t dev = 0; dev < mPhysicalDevices.size(); dev++) {
            std::string guid = mPhysicalDevices[dev]->GetGuid();
            if (guid != val.value()) {
                continue;
            }

            mVirtualDevices[slot] = dev;
        }
    }

    for (size_t i = 0; i < mVirtualDevices.size(); i++) {
        std::shared_ptr<Controller> backend = mPhysicalDevices[mVirtualDevices[i]];
        config->setString(StringHelper::Sprintf("Controllers.Deck.Slot_%d", (int)i), backend->GetGuid());
    }

    for (const auto device : mPhysicalDevices) {

        std::string guid = device->GetGuid();

        for (int32_t virtualSlot = 0; virtualSlot < MAXCONTROLLERS; virtualSlot++) {

            if (!(config->rjson["Controllers"].contains(guid) &&
                  config->rjson["Controllers"][guid].contains(StringHelper::Sprintf("Slot_%d", virtualSlot)))) {
                continue;
            }

            auto profile = device->getProfile(virtualSlot);
            auto rawProfile = config->rjson["Controllers"][guid][StringHelper::Sprintf("Slot_%d", virtualSlot)];

            profile->Mappings.clear();
            profile->AxisDeadzones.clear();
            profile->AxisDeadzones.clear();
            profile->GyroData.clear();

            profile->Version = config->getInt(NESTED("Version", ""), DEVICE_PROFILE_VERSION_V0);

            switch (profile->Version) {

                case DEVICE_PROFILE_VERSION_V0:

                    // Load up defaults for the things we can't load.
                    device->CreateDefaultBinding(virtualSlot);

                    profile->UseRumble = config->getBool(NESTED("Rumble.Enabled", ""));
                    profile->RumbleStrength = config->getFloat(NESTED("Rumble.Strength", ""));
                    profile->UseGyro = config->getBool(NESTED("Gyro.Enabled", ""));

                    for (auto const& val : rawProfile["Mappings"].items()) {
                        device->SetButtonMapping(virtualSlot, std::stoi(val.key().substr(4)), val.value());
                    }

                    break;

                case DEVICE_PROFILE_VERSION_V1:
                    profile->UseRumble = config->getBool(NESTED("Rumble.Enabled", ""));
                    profile->RumbleStrength = config->getFloat(NESTED("Rumble.Strength", ""));
                    profile->UseGyro = config->getBool(NESTED("Gyro.Enabled", ""));

                    for (auto const& val : rawProfile["AxisDeadzones"].items()) {
                        profile->AxisDeadzones[std::stoi(val.key())] = val.value();
                    }

                    for (auto const& val : rawProfile["AxisMinimumPress"].items()) {
                        profile->AxisMinimumPress[std::stoi(val.key())] = val.value();
                    }

                    for (auto const& val : rawProfile["GyroData"].items()) {
                        profile->GyroData[std::stoi(val.key())] = val.value();
                    }

                    for (auto const& val : rawProfile["Mappings"].items()) {
                        device->SetButtonMapping(virtualSlot, std::stoi(val.key().substr(4)), val.value());
                    }

                    break;

                // Version is invalid.
                default:
                    device->CreateDefaultBinding(virtualSlot);
                    break;
            }
        }
    }
}

void ControlDeck::SaveControllerSettings() {
    std::shared_ptr<Mercury> config = Window::GetInstance()->GetConfig();

    for (size_t i = 0; i < mVirtualDevices.size(); i++) {
        std::shared_ptr<Controller> backend = mPhysicalDevices[mVirtualDevices[i]];
        config->setString(StringHelper::Sprintf("Controllers.Deck.Slot_%d", (int)i), backend->GetGuid());
    }

    for (const auto& device : mPhysicalDevices) {

        int32_t virtualSlot = 0;
        std::string guid = device->GetGuid();

        for (int32_t virtualSlot = 0; virtualSlot < MAXCONTROLLERS; virtualSlot++) {
            auto profile = device->getProfile(virtualSlot);

            if (!device->Connected()) {
                continue;
            }

            auto rawProfile = config->rjson["Controllers"][guid][StringHelper::Sprintf("Slot_%d", virtualSlot)];
            config->setInt(NESTED("Version", ""), profile->Version);
            config->setBool(NESTED("Rumble.Enabled", ""), profile->UseRumble);
            config->setFloat(NESTED("Rumble.Strength", ""), profile->RumbleStrength);
            config->setBool(NESTED("Gyro.Enabled", ""), profile->UseGyro);

            for (auto const& val : rawProfile["Mappings"].items()) {
                config->setInt(NESTED("Mappings.%s", val.key().c_str()), -1);
            }

            for (auto const& [key, val] : profile->AxisDeadzones) {
                config->setFloat(NESTED("AxisDeadzones.%d", key), val);
            }

            for (auto const& [key, val] : profile->AxisMinimumPress) {
                config->setFloat(NESTED("AxisMinimumPress.%d", key), val);
            }

            for (auto const& [key, val] : profile->GyroData) {
                config->setFloat(NESTED("GyroData.%d", key), val);
            }

            for (auto const& [key, val] : profile->Mappings) {
                config->setInt(NESTED("Mappings.BTN_%d", val), key);
            }

            virtualSlot++;
        }
    }

    config->save();
}

std::shared_ptr<Controller> ControlDeck::GetPhysicalDevice(int32_t deviceSlot) {
    return mPhysicalDevices[deviceSlot];
}

size_t ControlDeck::GetNumPhysicalDevices() {
    return mPhysicalDevices.size();
}

int32_t ControlDeck::GetVirtualDevice(int32_t slot) {
    return mVirtualDevices[slot];
}

size_t ControlDeck::GetNumVirtualDevices() {
    return mVirtualDevices.size();
}

std::shared_ptr<Controller> ControlDeck::GetPhysicalDeviceFromVirtualSlot(int32_t slot) {
    return GetPhysicalDevice(GetVirtualDevice(slot));
}

uint8_t* ControlDeck::GetControllerBits() {
    return mControllerBits;
}

void ControlDeck::BlockGameInput() {
    mShouldBlockGameInput = true;
}

void ControlDeck::UnblockGameInput() {
    mShouldBlockGameInput = false;
}

bool ControlDeck::ShouldBlockGameInput(std::string inputDeviceGuid) const {
    // We block controller input if F1 menu is open and control navigation is on.
    // This is because we don't want controller inputs to affect the game
    bool shouldBlockControllerInput = CVarGetInteger("gOpenMenuBar", 0) && CVarGetInteger("gControlNav", 0);

    // We block keyboard input if you're currently typing into a textfield.
    // This is because we don't want your keyboard typing to affect the game.
    ImGuiIO io = ImGui::GetIO();
    bool shouldBlockKeyboardInput = io.WantCaptureKeyboard;

    bool inputDeviceIsKeyboard = inputDeviceGuid == "Keyboard";
    return mShouldBlockGameInput || (inputDeviceIsKeyboard ? shouldBlockKeyboardInput : shouldBlockControllerInput);
}
} // namespace Ship
