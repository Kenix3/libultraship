#ifdef __WIIU__
#include "WiiUMapping.h"
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
    auto deviceIndexMapping = std::static_pointer_cast<LUSDeviceIndexToWiiUDeviceIndexMapping>(
        LUS::Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromLUSDeviceIndex(mLUSDeviceIndex));

    if (deviceIndexMapping == nullptr) {
        // we don't have an sdl device for this LUS device index
        CloseController();
        return false;
    }

    if (deviceIndexMapping->IsWiiUGamepad()) {
        VPADReadError error;
        VPADStatus* status = LUS::WiiU::GetVPADStatus(&error);
        if (!status || error == VPAD_READ_INVALID_CONTROLLER) {
            CloseController();
            return false;
        }

        mController = nullptr;
        mWiiUGamepadController = status;
        return true;
    }

    KPADError error;
    KPADStatus* status = LUS::WiiU::GetKPADStatus(static_cast<WPADChan>(deviceIndexMapping->GetDeviceChannel()), &error);
    if (!status || error != KPAD_ERROR_OK) {
        CloseController();
        return false;
    }

    mController = status;
    mWiiUGamepadController = nullptr;

    return true;
}

bool WiiUMapping::CloseController() {
    mController = nullptr;
    mWiiUGamepadController = nullptr;

    return true;
}

bool WiiUMapping::ControllerLoaded() {
    if (IsGamepad()) {
        // If the controller is disconnected, close it.
        if (mWiiUGamepadController != nullptr && mWiiUGamepadController->error != VPAD_READ_SUCCESS) {
            CloseController();
        }

        // Attempt to load the controller if it's not loaded
        if (mWiiUGamepadController == nullptr) {
            // If we failed to load the controller, don't process it.
            if (!OpenController()) {
                return false;
            }
        }

        return true;
    }

    // If the controller is disconnected, close it.
    if (mController != nullptr && mController->error != KPAD_ERROR_OK) {
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

int32_t WiiUMapping::ExtensionType() {
    auto [isGamepad, extensionType] = LUS::Context::GetInstance()
                                          ->GetControlDeck()
                                          ->GetDeviceIndexMappingManager()
                                          ->GetWiiUDeviceTypeFromLUSDeviceIndex(mLUSDeviceIndex);

    return extensionType;
}

std::string WiiUMapping::GetWiiUDeviceName() {
    std::string deviceName = GetWiiUControllerName();

    if (!ControllerLoaded()) {
        deviceName += " (Disconnected)";
        return deviceName;
    }

    if (IsGamepad()) {
        return deviceName;
    }

    return StringHelper::Sprintf("%s (%d)", deviceName.c_str(), GetWiiUDeviceChannel() + 1);
}
} // namespace LUS
#endif
