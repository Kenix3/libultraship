#include "ship/controller/controldevice/controller/mapping/sdl/SDLButtonToAxisDirectionMapping.h"
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"
#include "ship/window/gui/IconsFontAwesome4.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/config/Config.h"

#define MAX_SDL_RANGE (float)INT16_MAX

namespace Ship {
SDLButtonToAxisDirectionMapping::SDLButtonToAxisDirectionMapping(uint8_t portIndex, StickIndex stickIndex,
                                                                 Direction direction, int32_t sdlControllerButton,
                                                                 std::shared_ptr<ControlDeck> controlDeck,
                                                                 std::shared_ptr<Config> config)
    : ControllerInputMapping(PhysicalDeviceType::SDLGamepad),
      ControllerAxisDirectionMapping(PhysicalDeviceType::SDLGamepad, portIndex, stickIndex, direction, controlDeck,
                                     config),
      SDLButtonToAnyMapping(sdlControllerButton) {
    mConsoleVariable = Ship::Context::GetInstance()->GetChildren().GetFirst<ConsoleVariable>();
    if (controlDeck) {
        mControlDeck = std::move(controlDeck);
    } else {
        mControlDeck = Context::GetInstance()->GetChildren().GetFirst<ControlDeck>();
    }
}

float SDLButtonToAxisDirectionMapping::GetNormalizedAxisDirectionValue() {
    if (mControlDeck->GamepadGameInputBlocked()) {
        return 0.0f;
    }

    for (const auto& [instanceId, gamepad] :
         mControlDeck->GetConnectedPhysicalDeviceManager()->GetConnectedSDLGamepadsForPort(mPortIndex)) {
        if (SDL_GameControllerGetButton(gamepad, mControllerButton)) {
            return MAX_AXIS_RANGE;
        }
    }

    return 0.0f;
}

std::string SDLButtonToAxisDirectionMapping::GetAxisDirectionMappingId() {
    return StringHelper::Sprintf("P%d-S%d-D%d-SDLB%d", mPortIndex, mStickIndex, mDirection, mControllerButton);
}

void SDLButtonToAxisDirectionMapping::SaveToConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    mConsoleVariable->SetString(StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str(),
                                "SDLButtonToAxisDirectionMapping");
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str(), mStickIndex);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str(), mDirection);
    mConsoleVariable->SetInteger(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str(),
                                 mControllerButton);
    mConsoleVariable->Save();
}

void SDLButtonToAxisDirectionMapping::EraseFromConfig() {
    const std::string mappingCvarKey = CVAR_PREFIX_CONTROLLERS ".AxisDirectionMappings." + GetAxisDirectionMappingId();
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Stick", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.Direction", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(
        StringHelper::Sprintf("%s.AxisDirectionMappingClass", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->ClearVariable(StringHelper::Sprintf("%s.SDLControllerButton", mappingCvarKey.c_str()).c_str());
    mConsoleVariable->Save();
}

int8_t SDLButtonToAxisDirectionMapping::GetMappingType() {
    return MAPPING_TYPE_GAMEPAD;
}

std::string SDLButtonToAxisDirectionMapping::GetPhysicalDeviceName() {
    return SDLButtonToAnyMapping::GetPhysicalDeviceName();
}

std::string SDLButtonToAxisDirectionMapping::GetPhysicalInputName() {
    return SDLButtonToAnyMapping::GetPhysicalInputName();
}
} // namespace Ship
