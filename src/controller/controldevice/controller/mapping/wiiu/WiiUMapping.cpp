#ifdef __WIIU__
#include "WiiUMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToWiiUDeviceIndexMapping.h"

#include <Utils/StringHelper.h>

namespace LUS {
WiiUMapping::WiiUMapping(LUSDeviceIndex lusDeviceIndex) : ControllerMapping(lusDeviceIndex) {
}

WiiUMapping::~WiiUMapping() {
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

    if (!WiiUDeviceIsConnected()) {
        deviceName += " (Disconnected)";
        return deviceName;
    }

    if (IsGamepad()) {
        return deviceName;
    }

    return StringHelper::Sprintf("%s (%d)", deviceName.c_str(), GetWiiUDeviceChannel() + 1);
}

bool WiiUMapping::WiiUDeviceIsConnected() {
    if (IsGamepad()) {
        VPADReadError error;
        VPADStatus* status = LUS::WiiU::GetVPADStatus(&error);
        if (status == nullptr) {
            return false;
        }

        return true;
    }

    KPADError error;
    KPADStatus* status = LUS::WiiU::GetKPADStatus(static_cast<KPADChan>(GetWiiUDeviceChannel()), &error);
    if (status == nullptr || error != KPAD_ERROR_OK) {
        return false;
    }

    return true;
}
} // namespace LUS
#endif
