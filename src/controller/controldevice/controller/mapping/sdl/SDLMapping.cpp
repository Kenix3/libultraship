#include "SDLMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"
#include "controller/deviceindex/ShipDeviceIndexToSDLDeviceIndexMapping.h"

#include "utils/StringHelper.h"

namespace Ship {
SDLMapping::SDLMapping(ShipDeviceIndex shipDeviceIndex) : ControllerMapping(shipDeviceIndex), mController(nullptr) {
}

SDLMapping::~SDLMapping() {
}

bool SDLMapping::OpenController() {
    auto deviceIndexMapping = std::static_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(
        Ship::Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromShipDeviceIndex(mShipDeviceIndex));

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

bool SDLMapping::CloseController() {
    if (mController != nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_GameControllerClose(mController);
    }
    mController = nullptr;

    return true;
}

bool SDLMapping::ControllerLoaded(bool closeIfDisconnected) {
    SDL_GameControllerUpdate();

    // If the controller is disconnected
    if (mController != nullptr && !SDL_GameControllerGetAttached(mController)) {
        if (!closeIfDisconnected) {
            return false;
        }

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

SDL_GameControllerType SDLMapping::GetSDLControllerType() {
    if (!ControllerLoaded()) {
        return SDL_CONTROLLER_TYPE_UNKNOWN;
    }

    return SDL_GameControllerGetType(mController);
}

uint16_t SDLMapping::GetSDLControllerVendorId() {
    if (!ControllerLoaded()) {
        return 0;
    }

    return SDL_GameControllerGetVendor(mController);
}

uint16_t SDLMapping::GetSDLControllerProductId() {
    if (!ControllerLoaded()) {
        return 0;
    }

    return SDL_GameControllerGetProduct(mController);
}

bool SDLMapping::UsesPlaystationLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_PS3 || type == SDL_CONTROLLER_TYPE_PS4 || type == SDL_CONTROLLER_TYPE_PS5;
}

bool SDLMapping::UsesSwitchLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO || type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
}

bool SDLMapping::UsesXboxLayout() {
    auto type = GetSDLControllerType();
    return type == SDL_CONTROLLER_TYPE_XBOX360 || type == SDL_CONTROLLER_TYPE_XBOXONE;
}

bool SDLMapping::UsesGameCubeLayout() {
    auto vid = GetSDLControllerVendorId();
    auto pid = GetSDLControllerProductId();

    return vid == 0x57e && pid == 0x337;
}

int32_t SDLMapping::GetSDLDeviceIndex() {
    auto deviceIndexMapping = std::static_pointer_cast<ShipDeviceIndexToSDLDeviceIndexMapping>(
        Ship::Context::GetInstance()
            ->GetControlDeck()
            ->GetDeviceIndexMappingManager()
            ->GetDeviceIndexMappingFromShipDeviceIndex(mShipDeviceIndex));

    if (deviceIndexMapping == nullptr) {
        // we don't have an sdl device for this LUS device index
        return -1;
    }

    return deviceIndexMapping->GetSDLDeviceIndex();
}

std::string SDLMapping::GetSDLControllerName() {
    return Ship::Context::GetInstance()
        ->GetControlDeck()
        ->GetDeviceIndexMappingManager()
        ->GetSDLControllerNameFromShipDeviceIndex(mShipDeviceIndex);
}

std::string SDLMapping::GetSDLDeviceName() {
    return ControllerLoaded()
               ? StringHelper::Sprintf("%s (SDL %d)", GetSDLControllerName().c_str(), GetSDLDeviceIndex())
               : StringHelper::Sprintf("%s (Disconnected)", GetSDLControllerName().c_str());
}

int32_t SDLMapping::GetJoystickInstanceId() {
    if (mController == nullptr) {
        return -1;
    }

    return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(mController));
}

int32_t SDLMapping::GetCurrentSDLDeviceIndex() {
    if (mController == nullptr) {
        return -1;
    }

    for (int32_t i = 0; i < SDL_NumJoysticks(); i++) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        if (SDL_JoystickInstanceID(joystick) == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(mController))) {
            SDL_JoystickClose(joystick);
            return i;
        }
        SDL_JoystickClose(joystick);
    }

    // didn't find one
    return -1;
}
} // namespace Ship
