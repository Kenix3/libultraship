#include "libultraship/controller/controldeck/ControlDeck.h"

#include "ship/Context.h"
#include "libultraship/controller/controldevice/controller/Controller.h"
#include "libultraship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"
#include "ship/utils/StringHelper.h"
#include <imgui.h>
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"

namespace LUS {
ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                         std::shared_ptr<Ship::ControllerDefaultMappings> controllerDefaultMappings,
                         std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames)
    : Ship::ControlDeck(additionalBitmasks, controllerDefaultMappings, buttonNames), mPads(nullptr) {
    std::vector<CONTROLLERBUTTONS_T> bitmasks;
    for (auto [bitmask, name] : buttonNames) {
        bitmasks.push_back(bitmask);
    }
    bitmasks.insert(bitmasks.end(), additionalBitmasks.begin(), additionalBitmasks.end());
    for (int32_t i = 0; i < MAXCONTROLLERS; i++) {
        mPorts.push_back(std::make_shared<Ship::ControlPort>(i, std::make_shared<Controller>(i, bitmasks)));
    }
}

ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks)
    : ControlDeck(additionalBitmasks, std::make_shared<LUS::ControllerDefaultMappings>(),
                  std::unordered_map<CONTROLLERBUTTONS_T, std::string>({
                      { BTN_A, "A" },
                      { BTN_B, "B" },
                      { BTN_L, "L" },
                      { BTN_R, "R" },
                      { BTN_Z, "Z" },
                      { BTN_START, "Start" },
                      { BTN_CLEFT, "CLeft" },
                      { BTN_CRIGHT, "CRight" },
                      { BTN_CUP, "CUp" },
                      { BTN_CDOWN, "CDown" },
                      { BTN_DLEFT, "DLeft" },
                      { BTN_DRIGHT, "DRight" },
                      { BTN_DUP, "DUp" },
                      { BTN_DDOWN, "DDown" },
                  })) {
}

ControlDeck::ControlDeck() : ControlDeck(std::vector<CONTROLLERBUTTONS_T>()) {
}

Ship::OnScreenKeyboardBindings ControlDeck::GetOnScreenKeyboardBindings() {
    Ship::OnScreenKeyboardBindings bindings;
    bindings.Type = GetGamepadBinding(0, BTN_A);
    bindings.Remove = GetGamepadBinding(0, BTN_B);
    bindings.Shift = GetGamepadBinding(0, BTN_CUP);
    bindings.Space = GetGamepadBinding(0, BTN_CDOWN);
    bindings.CursorLeft = GetGamepadBinding(0, BTN_L);
    bindings.CursorRight = GetGamepadBinding(0, BTN_R);
    bindings.Language = GetGamepadBinding(0, BTN_Z);
    bindings.Done = GetGamepadBinding(0, BTN_START);
    bindings.Up = GetGamepadBinding(0, BTN_DUP);
    bindings.Down = GetGamepadBinding(0, BTN_DDOWN);
    bindings.Left = GetGamepadBinding(0, BTN_DLEFT);
    bindings.Right = GetGamepadBinding(0, BTN_DRIGHT);
    return bindings;
}

OSContPad* ControlDeck::GetPads() {
    return mPads;
}

void ControlDeck::WriteToPad(void* pad) {
    WriteToOSContPad((OSContPad*)pad);
}

void ControlDeck::WriteToOSContPad(OSContPad* pad) {
    SDL_PumpEvents();
    Ship::WheelHandler::GetInstance()->Update();

    if (AllGameInputBlocked()) {
        return;
    }

    mPads = pad;

    for (size_t i = 0; i < mPorts.size(); i++) {
        const std::shared_ptr<Ship::Controller> controller = mPorts[i]->GetConnectedController();

        if (controller != nullptr) {
            controller->ReadToPad(&pad[i]);
        }
    }
}
} // namespace LUS
