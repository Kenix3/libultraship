#include "DummyController.h"

namespace Ship {
DummyController::DummyController(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex, const std::string& guid, const std::string& keyName, bool connected) : Controller(controlDeck, deviceIndex) {
    mGuid = guid;
    mControllerName = guid;
    mIsConnected = connected;
    mButtonName = keyName;
}

void DummyController::ReadFromSource(int32_t portIndex) {
}

const std::string DummyController::GetButtonName(int32_t portIndex, int32_t n64Button) {
    return mButtonName;
}

void DummyController::WriteToSource(int32_t portIndex, ControllerCallback* controller) {
}

bool DummyController::Connected() const {
    return mIsConnected;
}

bool DummyController::CanRumble() const {
    return false;
}

bool DummyController::CanGyro() const {
    return false;
}

void DummyController::CreateDefaultBinding(int32_t portIndex) {
}

std::string DummyController::GetControllerType() {
    return "Unk";
}

std::string DummyController::GetConfSection() {
    return "Unk";
}

std::string DummyController::GetBindingConfSection() {
    return "Unk";
}

void DummyController::ClearRawPress() {
}

int32_t DummyController::ReadRawPress() {
    return -1;
}
} // namespace Ship
