#include "SDLMapping.h"
#include <spdlog/spdlog.h>
#include "Context.h"
#include "controller/deviceindex/LUSDeviceIndexToSDLDeviceIndexMapping.h"

#include <Utils/StringHelper.h>

namespace LUS {
SDLMapping::SDLMapping(LUSDeviceIndex lusDeviceIndex) : ControllerMapping(lusDeviceIndex), mController(nullptr) {
}

SDLMapping::~SDLMapping() {
}

bool SDLMapping::OpenController() {
    auto deviceIndexMapping = std::static_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(LUS::Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetDeviceIndexMappingFromLUSDeviceIndex(mLUSDeviceIndex));

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

bool SDLMapping::ControllerLoaded() {
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

SDL_GameControllerType SDLMapping::GetSDLControllerType() {
    if (!ControllerLoaded()) {
        return SDL_CONTROLLER_TYPE_UNKNOWN;
    }

    return SDL_GameControllerGetType(mController);
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

int32_t SDLMapping::GetSDLDeviceIndex() {
    auto deviceIndexMapping = std::static_pointer_cast<LUSDeviceIndexToSDLDeviceIndexMapping>(LUS::Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->GetDeviceIndexMappingFromLUSDeviceIndex(mLUSDeviceIndex));

    if (deviceIndexMapping == nullptr) {
        // we don't have an sdl device for this LUS device index
        return -1;
    }

    return deviceIndexMapping->GetSDLDeviceIndex();
}

std::string SDLMapping::GetSDLDeviceName() {
    if (!ControllerLoaded()) {
        return StringHelper::Sprintf("Disconnected (%d)", mLUSDeviceIndex);    
    }

    auto sdlName = SDL_GameControllerName(mController);
    return StringHelper::Sprintf("%s (SDL %d)", sdlName != nullptr ? sdlName : "Unknown", GetSDLDeviceIndex());
}

int32_t SDLMapping::GetJoystickInstanceId() {
    if (mController == nullptr) {
        return -1;
    }

    return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(mController));
}
} // namespace LUS
