#include "DummyController.h"

namespace Ship {
DummyController::DummyController(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex, const std::string& guid,
                                 const std::string& keyName, bool connected)
    : Controller(controlDeck, deviceIndex) {
    mGuid = guid;
    mControllerName = guid;
    mIsConnected = connected;
    mButtonName = keyName;
}

void DummyController::ReadDevice(int32_t portIndex) {
}

const std::string DummyController::GetButtonName(int32_t portIndex, int32_t n64Button) {
    return mButtonName;
}

bool DummyController::Connected() const {
    return mIsConnected;
}

int32_t DummyController::SetRumble(int32_t portIndex, bool rumble) {
    return -1000;
}

int32_t DummyController::SetLed(int32_t portIndex, int8_t r, int8_t g, int8_t b) {
    return -1000;
}

bool DummyController::CanSetLed() const {
    return false;
}

bool DummyController::CanRumble() const {
    return false;
}

bool DummyController::CanGyro() const {
    return false;
}

void DummyController::CreateDefaultBinding(int32_t portIndex) {
}

void DummyController::ClearRawPress() {
}

int32_t DummyController::ReadRawPress() {
    return -1;
}
} // namespace Ship
