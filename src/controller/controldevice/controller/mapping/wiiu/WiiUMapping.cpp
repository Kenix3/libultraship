#ifdef __WIIU__
#include "SDLMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToWiiUDeviceIndexMapping.h"

#include <Utils/StringHelper.h>

namespace LUS {
WiiUMapping::WiiUMapping(LUSDeviceIndex lusDeviceIndex) : ControllerMapping(lusDeviceIndex), mDeviceConnected(false) {
}

WiiUMapping::~WiiUMapping() {
}

bool WiiUMapping::OpenController() {
    auto deviceIndexMapping = std::static_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(
        LUS::Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(mLUSDeviceIndex));

    if (deviceIndexMapping == nullptr) {
        // we don't have an sdl device for this LUS device index
        mController = nullptr;
        return false;
    }

    const auto newCont = SDL_GameControllerOpen(deviceIndexMapping->GetSDLDeviceIndex());

    // We failed to load the controller
    if (newCont == nullptr) {
        return false;
    }

    mController = newCont;
    return true;
}

bool WiiUMapping::CloseController() {
    if (mController != nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_GameControllerClose(mController);
    }
    mController = nullptr;

    return true;
}

bool WiiUMapping::ControllerLoaded() {
    SDL_GameControllerUpdate();

    // If the controller is disconnected, close it.
    if (mController != nullptr && !SDL_GameControllerGetAttached(mController)) {
        CloseController();
    }

    // Attempt to load the controller if it's not loaded
    if (mController == nullptr) {
        // If we failed to load the controller, don't process it.
        if (!OpenController()) {
            return false;
        }
    }

    return true;
}

int32_t WiiUMapping::GetWiiUDeviceChannel() {
    auto deviceIndexMapping = std::static_pointer_cast<LUSDeviceIndexToWiiUDeviceIndexMapping>(
        LUS::Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(mLUSDeviceIndex));

    if (deviceIndexMapping == nullptr) {
        // we don't have a wii u device for this LUS device index
        return -1;
    }

    return deviceIndexMapping->GetDeviceChannel();
}

std::string WiiUMapping::GetWiiUControllerName() {
    auto [isGamepad, extensionType] = LUS::Context::GetInstance()
        ->GetControlDeck()
        ->GetDeviceIndexMappingManager()
        ->GetWiiUDeviceTypeFromLUSDeviceIndex(mLUSDeviceIndex);

    if (isGamepad) {
        return "Wii U Gamepad";
    }

    switch (extensionType) {
        case WPAD_EXT_CORE:
            return "Wii Remote";
        case WPAD_EXT_NUNCHUK:
            return "Wii Remote + Nunchuk";
        case WPAD_EXT_MPLUS_CLASSIC:
        case WPAD_EXT_CLASSIC:
            return "Wii Classic Controller";
        case WPAD_EXT_MPLUS:
            return "Motion Plus Wii Remote";
        case WPAD_EXT_MPLUS_NUNCHUK:
            return "Motion Plus Wii Remote + Nunchuk";
        case WPAD_EXT_PRO_CONTROLLER:
            return "Wii U Pro Controller";
        default:
            return "Unknown";
    }
}

bool WiiUMapping::IsGamepad() {
    auto [isGamepad, extensionType] = LUS::Context::GetInstance()
        ->GetControlDeck()
        ->GetDeviceIndexMappingManager()
        ->GetWiiUDeviceTypeFromLUSDeviceIndex(mLUSDeviceIndex);

    return isGamepad;
}

std::string WiiUMapping::GetWiiUDeviceName() {
    std::string deviceName = GetSDLControllerName();

    if (!ControllerLoaded()) {
        deviceName += " (Disconnected)";
        return deviceName;
    }

    if (IsGamepad()) {
        return deviceName;
    }

    return StringHelper::Sprintf("%s (%d)", deviceName.c_str(), GetDeviceChannel() + 1);
}
} // namespace LUS
#endif
