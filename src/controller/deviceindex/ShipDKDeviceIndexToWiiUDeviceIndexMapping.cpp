#ifdef __WIIU__
#include "ShipDKDeviceIndexToWiiUDeviceIndexMapping.h"
#include <Utils/StringHelper.h>
#include "public/bridge/consolevariablebridge.h"

namespace ShipDK {
ShipDKDeviceIndexToWiiUDeviceIndexMapping::ShipDKDeviceIndexToWiiUDeviceIndexMapping(ShipDKDeviceIndex shipDKDeviceIndex,
                                                                               bool isGamepad, int32_t deviceChannel,
                                                                               int32_t extensionType)
    : ShipDKDeviceIndexToPhysicalDeviceIndexMapping(shipDKDeviceIndex), mIsWiiUGamepad(isGamepad),
      mDeviceChannel(deviceChannel), mExtensionType(extensionType) {
}

ShipDKDeviceIndexToWiiUDeviceIndexMapping::~ShipDKDeviceIndexToWiiUDeviceIndexMapping() {
}

bool ShipDKDeviceIndexToWiiUDeviceIndexMapping::IsWiiUGamepad() {
    return mIsWiiUGamepad;
}

int32_t shipDKDeviceIndexToWiiUDeviceIndexMapping::GetDeviceChannel() {
    return mDeviceChannel;
}

void ShipDKDeviceIndexToWiiUDeviceIndexMapping::SetDeviceChannel(int32_t channel) {
    mDeviceChannel = channel;
}

int32_t shipDKDeviceIndexToWiiUDeviceIndexMapping::GetExtensionType() {
    return mExtensionType;
}

void ShipDKDeviceIndexToWiiUDeviceIndexMapping::SetExtensionType(int32_t extensionType) {
    mExtensionType = extensionType;
}

bool ShipDKDeviceIndexToWiiUDeviceIndexMapping::HasEquivalentExtensionType(int32_t extensionType) {
    switch (extensionType) {
        case WPAD_EXT_CORE:  // Wii Remote with no extension
        case WPAD_EXT_MPLUS: // Wii remote with motion plus
            return mExtensionType == WPAD_EXT_CORE || mExtensionType == WPAD_EXT_MPLUS;
        case WPAD_EXT_NUNCHUK:       // Wii Remote with nunchuk
        case WPAD_EXT_MPLUS_NUNCHUK: // Wii remote with motion plus and nunchuk
            return mExtensionType == WPAD_EXT_NUNCHUK || mExtensionType == WPAD_EXT_MPLUS_NUNCHUK;
        case WPAD_EXT_CLASSIC:       // Wii Remote with Classic Controller
        case WPAD_EXT_MPLUS_CLASSIC: // Wii Remote with motion plus and Classic Controller
            return mExtensionType == WPAD_EXT_CLASSIC || mExtensionType == WPAD_EXT_MPLUS_CLASSIC;
        case WPAD_EXT_PRO_CONTROLLER: // Wii U Pro Controller
            return mExtensionType == WPAD_EXT_PRO_CONTROLLER;
        default:
            return false;
    }
}

std::string ShipDKDeviceIndexToWiiUDeviceIndexMapping::GetWiiUControllerName() {
    if (IsWiiUGamepad()) {
        return "Wii U Gamepad";
    }

    switch (GetExtensionType()) {
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

void ShipDKDeviceIndexToWiiUDeviceIndexMapping::SaveToConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();
    CVarSetString(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str(),
                  "ShipDKDeviceIndexToWiiUDeviceIndexMapping");
    CVarSetInteger(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str(), mShipDKDeviceIndex);
    CVarSetInteger(StringHelper::Sprintf("%s.IsGamepad", mappingCvarKey.c_str()).c_str(), mIsWiiUGamepad);
    CVarSetInteger(StringHelper::Sprintf("%s.WiiUDeviceChannel", mappingCvarKey.c_str()).c_str(), mDeviceChannel);
    CVarSetInteger(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str(), mExtensionType);
    CVarSave();
}

void ShipDKDeviceIndexToWiiUDeviceIndexMapping::EraseFromConfig() {
    const std::string mappingCvarKey = "gControllers.DeviceMappings." + GetMappingId();

    CVarClear(StringHelper::Sprintf("%s.DeviceMappingClass", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.ShipDKDeviceIndex", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.IsGamepad", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WiiUDeviceChannel", mappingCvarKey.c_str()).c_str());
    CVarClear(StringHelper::Sprintf("%s.WiiUDeviceExtensionType", mappingCvarKey.c_str()).c_str());

    CVarSave();
}
} // namespace ShipDK
#endif
